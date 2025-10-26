#include "nfc_reader.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <string>
#include <cstdint>

extern "C" {
#include <nfc/nfc.h>
}
#include <array>
extern bool nfc_read_uid(uint8_t* out, int len, int timeout);

static const nfc_modulation kMods[] = {
    { NMT_ISO14443A, NBR_106 },
};

namespace sca {

static std::string to_hex(const uint8_t* d, size_t n) {
    static const char* H = "0123456789ABCDEF";
    std::string s; s.reserve(n * 2);
    for (size_t i=0;i<n;++i){ s.push_back(H[d[i]>>4]); s.push_back(H[d[i]&0xF]); }
    return s;
}

static std::string normalize_hex(std::string s) {
    // 공백/하이픈 제거 + 대문자화
    std::string t; t.reserve(s.size());
    for (char c: s) {
        if (c==' ' || c=='-') continue;
        t.push_back((char)std::toupper((unsigned char)c));
    }
    return t;
}
static void dump_target_basic(const nfc_target& nt) {
     switch (nt.nm.nmt) {
     case NMT_ISO14443A:
         std::cerr << "[NFC] RAW ISO14443A:"
                   << " ATQA=" << std::hex
                   << (int)nt.nti.nai.abtAtqa[0] << (int)nt.nti.nai.abtAtqa[1]
                   << std::dec
                   << " SAK=" << (int)nt.nti.nai.btSak
                  << " UIDLen=" << (int)nt.nti.nai.szUidLen
                   << "\n";
         break;
     default:
         std::cerr << "[NFC] RAW unknown type\n";
         break;
     }
 }
bool nfc_poll_once(const NfcConfig& cfg, NfcResult& out) {
    out = NfcResult{};  // ok=false, uid_hex=""

    nfc_context* ctx = nullptr;
    nfc_init(&ctx);
    if (!ctx) { std::cerr << "[NFC] nfc_init failed\n"; return false; }

    nfc_device* pnd = nfc_open(ctx, nullptr);
    if (!pnd) { std::cerr << "[NFC] nfc_open failed\n"; nfc_exit(ctx); return false; }

    if (nfc_initiator_init(pnd) < 0) {
        std::cerr << "[NFC] nfc_initiator_init failed\n";
        nfc_close(pnd); nfc_exit(ctx); return false;
    }

    const int uiPollNr = 1;
   const int uiPeriodMs = 150; // 150~200ms 권장

    // ★ 블로킹 원인 제거(핵심 3줄) + 안전 타임아웃
    nfc_device_set_property_bool(pnd, NP_INFINITE_SELECT, false);   // 무한 select 금지
    nfc_device_set_property_bool(pnd, NP_ACTIVATE_FIELD,  true);    // 안테나 필드 ON
    nfc_device_set_property_int (pnd, NP_TIMEOUT_COMMAND, 200);     // 명령 타임아웃
    nfc_device_set_property_int (pnd, NP_TIMEOUT_ATR,     200);     // ATR 타임아웃


    const int poll_seconds = (cfg.poll_seconds > 0 ? cfg.poll_seconds : 5);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(poll_seconds);

    bool ever_detected = false;

    while (std::chrono::steady_clock::now() < deadline) {
        nfc_target nt{};
        // 테스트 코드와 동일 파라미터: 2회, 150ms
        int rc = nfc_initiator_poll_target(pnd, kMods, 1, /*uiPollNr*/2, /*uiPeriod*/200, &nt);
        if (rc > 0) {
            ever_detected = true;
            std::string uid_hex;
            switch (nt.nm.nmt) {
            case NMT_ISO14443A:
                if (nt.nti.nai.szUidLen > 0) {
                    uid_hex = to_hex(nt.nti.nai.abtUid, nt.nti.nai.szUidLen);
                    std::cout << "[NFC] ISO14443A UID=" << uid_hex << "\n";
                } else {
                    std::cout << "[NFC] ISO14443A detected (UID len=0)\n";
                }
                break;
            default:
                dump_target_basic(nt);
                break;
            }

            // 선택 해제는 한 번만
            nfc_initiator_deselect_target(pnd);

            // UID를 얻었으면 여기서 결과 확정
            if (!uid_hex.empty()) {
                out.ok = true;
                out.uid_hex = uid_hex;
                break;
            }
        }
        else if (rc == 0) {
            std::cerr << "[NFC] polling...\n";
        }
        else { // rc < 0
            std::cerr << "[NFC] poll error: " << nfc_strerror(pnd) << "\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(uiPeriodMs));
    }

    if (!out.ok) {
        if (ever_detected) {
            std::cerr << "[NFC] 태그 감지했지만 UID를 읽지 못함\n";
        } else {
            std::cerr << "[NFC] 제한시간(" << poll_seconds << "s) 내 미검출\n";
        }
    }

    nfc_close(pnd);
    nfc_exit(ctx);
    return out.ok;
}

} // namespace sca
