#include "nfc_reader.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>
extern "C" {
#include <nfc/nfc.h>
}
#include <array>
extern bool nfc_read_uid(uint8_t* out, int len, int timeout);
static const uint8_t AID[] = { 0xF0,0x01,0x02,0x03,0x04,0x05,0x06 };
static const size_t AID_LEN = sizeof(AID);
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
// ISO-DEP APDU 교환 (SELECT AID → 응답 0x9000이면 out_hce_hex에 데이터 저장)
static bool try_apdu_exchange(nfc_device * pnd, std::string & out_hce_hex) {
    uint8_t select_apdu[5 + AID_LEN];
    select_apdu[0] = 0x00; // CLA
    select_apdu[1] = 0xA4; // INS (SELECT)
    select_apdu[2] = 0x04; // P1
    select_apdu[3] = 0x00; // P2
    select_apdu[4] = AID_LEN; // Lc
    std::memcpy(select_apdu + 5, AID, AID_LEN);
    
    uint8_t resp[264];
    int rlen = nfc_initiator_transceive_bytes(pnd, select_apdu, sizeof(select_apdu),
    resp, sizeof(resp), 500);
    if (rlen < 2) {
        std::cerr << "[NFC] APDU failed len=" << rlen << "\n";
        return false;
    }
    uint8_t sw1 = resp[rlen - 2], sw2 = resp[rlen - 1];
    std::cout << "[NFC] APDU resp (" << rlen << "B): ";
    for (int i = 0; i < rlen; i++) printf("%02X ", resp[i]);
    std::cout << "\n";
    if (sw1 == 0x90 && sw2 == 0x00) {
        int dlen = rlen - 2;
        if (dlen > 0) {
            out_hce_hex = to_hex(resp, dlen);
            return true;
        }    
    }
    return false;
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

    // 블로킹 최소화 & ISO-DEP/APDU 준비
    nfc_device_set_property_bool(pnd, NP_INFINITE_SELECT, false);
    nfc_device_set_property_bool(pnd, NP_ACTIVATE_FIELD, true);
    nfc_device_set_property_int(pnd, NP_TIMEOUT_COMMAND, 500);
    nfc_device_set_property_int(pnd, NP_TIMEOUT_ATR, 200);
    nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, true);
    nfc_device_set_property_bool(pnd, NP_AUTO_ISO14443_4, true);

    const nfc_modulation mod = { NMT_ISO14443A, NBR_106 };

    const int poll_seconds = (cfg.poll_seconds > 0 ? cfg.poll_seconds : 5);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(poll_seconds);

    bool ever_detected = false;
    int  iter = 0;

    while (std::chrono::steady_clock::now() < deadline) {
        nfc_target nt{};
        int rc = nfc_initiator_select_passive_target(pnd, mod,
            /*pbtInitData*/nullptr, 0, &nt);
        if (rc > 0) {
            ever_detected = true;

            std::string uid_hex, hce_hex;
            if (nt.nm.nmt == NMT_ISO14443A && nt.nti.nai.szUidLen > 0) {
                uid_hex = to_hex(nt.nti.nai.abtUid, nt.nti.nai.szUidLen);
                // std::cout << "[NFC] UID=" << uid_hex << "\n";
            }

            if (try_apdu_exchange(pnd, hce_hex)) {
                out.ok = true; out.uid_hex = hce_hex; out.use_apdu = true;
            }
            else if (!uid_hex.empty()) {
                out.ok = true; out.uid_hex = uid_hex; out.use_apdu = false;
            }

            nfc_initiator_deselect_target(pnd);
            if (out.ok) break;
        }
        else if (rc == 0) {

        }
        else {
            // std::cerr << "[NFC] select err: " << nfc_strerror(pnd) << "\n";
            nfc_initiator_target_is_present(pnd, nullptr); 
            nfc_initiator_init(pnd);
            nfc_device_set_property_bool(pnd, NP_ACTIVATE_FIELD, true);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        ++iter;

        if ((iter % 12) == 0) {
            nfc_device_set_property_bool(pnd, NP_ACTIVATE_FIELD, false);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            nfc_device_set_property_bool(pnd, NP_ACTIVATE_FIELD, true);
        }
    }

    if (!out.ok) {
        if (ever_detected) std::cerr << "[NFC] 태그 감지했지만 UID/APDU 못 읽음\n";
        else               std::cerr << "[NFC] 제한시간(" << poll_seconds << "s) 내 미검출\n";
    }

    nfc_close(pnd);
    nfc_exit(ctx);
    return out.ok;
}

} // namespace sca
