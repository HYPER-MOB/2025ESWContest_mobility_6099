#include "nfc_reader.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <string>

extern "C" {
#include <nfc/nfc.h>
}

static const nfc_modulation kMods[] = {
    { NMT_ISO14443A, NBR_106 },
    { NMT_FELICA,    NBR_212 },
    { NMT_FELICA,    NBR_424 },
    { NMT_ISO14443B, NBR_106 },
    { NMT_JEWEL,     NBR_106 },
};

namespace sca {

static std::string to_hex(const uint8_t* d, size_t n) {
    static const char* H = "0123456789ABCDEF";
    std::string s; s.reserve(n * 2);
    for (size_t i=0;i<n;++i){ s.push_back(H[d[i]>>4]); s.push_back(H[d[i]&0xF]); }
    return s;
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
    case NMT_ISO14443B:
        std::cerr << "[NFC] RAW ISO14443B:"
                  << " PUPI=" << std::hex
                  << (int)nt.nti.nbi.abtPupi[0] << (int)nt.nti.nbi.abtPupi[1]
                  << (int)nt.nti.nbi.abtPupi[2] << (int)nt.nti.nbi.abtPupi[3]
                  << std::dec << "\n";
        break;
    case NMT_FELICA:
        std::cerr << "[NFC] RAW FeliCa: Len=" << (int)nt.nti.nfi.szLen
                  << " (IDm는 아래 별도 출력)\n";
        break;
    case NMT_JEWEL:
        std::cerr << "[NFC] RAW Jewel(Topaz)\n";
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
    const int poll_seconds = (cfg.poll_seconds > 0 ? cfg.poll_seconds : 5);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(poll_seconds);

    bool ever_detected = false;  // 한 번이라도 태그를 봤는지

    while (std::chrono::steady_clock::now() < deadline) {
        nfc_target nt{};

        int res = nfc_initiator_poll_target(
            pnd,
            kMods,
            sizeof(kMods)/sizeof(kMods[0]),
            /*uiPollNr*/ 2,
            /*uiPeriod*/ 200,
            &nt
        );

        if (res > 0) {
    ever_detected = true;

    std::string uid_hex;
    switch (nt.nm.nmt) {
    case NMT_ISO14443A:
        if (nt.nti.nai.szUidLen > 0) {
            uid_hex = to_hex(nt.nti.nai.abtUid, nt.nti.nai.szUidLen);
            std::cout << "[NFC] ISO14443A UID=" << uid_hex << "\n";
        } else {
            std::cout << "[NFC] ISO14443A detected (UID 길이 0)\n";
        }
        break;

    case NMT_ISO14443B:
        // ISO14443B: PUPI(4바이트)
        uid_hex = to_hex(nt.nti.nbi.abtPupi, 4);
        std::cout << "[NFC] ISO14443B PUPI=" << uid_hex << "\n";
        break;

    case NMT_FELICA:
        // FeliCa(IDm) 8바이트
        uid_hex = to_hex(nt.nti.nfi.abtId, 8);
        std::cout << "[NFC] FeliCa IDm=" << uid_hex << "\n";
        break;

    case NMT_JEWEL:
        // ★ 버전별 필드명: abtId 아님! btId 사용
        uid_hex = to_hex(nt.nti.nji.btId, 4);
        std::cout << "[NFC] Jewel ID=" << uid_hex << "\n";
        break;

    default:
        // 라이브러리 to_string이 없는 버전 대응: 최소 정보 수동 덤프
        dump_target_basic(nt);
        break;
    }

    if (!uid_hex.empty()) {
        out.ok = true;
        out.uid_hex = uid_hex;
        nfc_initiator_deselect_target(pnd);
        break;
    }

    nfc_initiator_deselect_target(pnd);

            // UID류가 있다면 성공 처리
            if (!uid_hex.empty()) {
                out.ok = true;
                out.uid_hex = uid_hex;
                std::cout << "[NFC] UUID" << uid_hex << "\n";
                // 다음 폴링을 위해 선택 해제(연속 동작 시 권장)
                nfc_initiator_deselect_target(pnd);
                break;
            }

            nfc_initiator_deselect_target(pnd);
        }
        else if (res == 0) {
            // 타임슬롯 내 미검출: 너무 시끄럽지 않게 간단한 dot pace
            static int dot = 0;
            if ((dot++ % 4) == 0) std::cout << "[NFC] polling...\n";
        }
        else { // res < 0
            std::cerr << "[NFC] poll error (res=" << res << ")\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    if (!out.ok) {
        if (ever_detected) {
            std::cout << "[NFC] 태그를 감지했지만 UID를 읽지 못했습니다.\n";
        } else {
            std::cout << "[NFC] 제한시간(" << poll_seconds << "s) 내에 태그를 전혀 감지하지 못했습니다.\n";
        }
    }

    nfc_close(pnd);
    nfc_exit(ctx);
    return out.ok;
}

} // namespace sca
