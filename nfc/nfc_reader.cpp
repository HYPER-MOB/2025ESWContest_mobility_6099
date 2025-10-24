#include "nfc_reader.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <cctype>

extern "C" {
#include <nfc/nfc.h>
}

namespace sca {

    static std::string hex(const uint8_t* d, size_t n) {
        static const char* k = "0123456789ABCDEF";
        std::string s; s.reserve(n * 2);
        for (size_t i = 0; i < n; ++i) { s.push_back(k[d[i] >> 4]); s.push_back(k[d[i] & 0xF]); }
        return s;
    }

    bool nfc_poll_once(const NfcConfig& cfg, NfcResult& out) {
        out = NfcResult{};
        nfc_context* ctx = nullptr;
        nfc_init(&ctx);
        if (!ctx) { std::cerr << "[NFC] nfc_init failed\n"; return false; }

        nfc_device* pnd = nfc_open(ctx, nullptr);
        if (!pnd) { std::cerr << "[NFC] nfc_open failed\n"; nfc_exit(ctx); return false; }

        if (nfc_initiator_init(pnd) < 0) {
            std::cerr << "[NFC] nfc_initiator_init failed\n";
            nfc_close(pnd); nfc_exit(ctx); return false;
        }

        const nfc_modulation mods[] = {
          { NMT_ISO14443A, NBR_106 },
          { NMT_FELICA,    NBR_212 },
          { NMT_FELICA,    NBR_424 },
          { NMT_ISO14443B, NBR_106 },
          { NMT_JEWEL,     NBR_106 },
        };

        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(cfg.poll_seconds);
        while (std::chrono::steady_clock::now() < deadline) {
            nfc_target nt{};
            int res = nfc_initiator_poll_target(pnd, mods, sizeof(mods) / sizeof(mods[0]),
                2,  /* 각 모드 타임슬롯 */
                200 /* ms 타임아웃 */);
            if (res > 0) {
                if (nt.nti.nai.szUidLen) {
                    out.ok = true;
                    out.uid_hex = hex(nt.nti.nai.abtUid, nt.nti.nai.szUidLen);
                    std::cout << "[NFC] UID=" << out.uid_hex << "\n";
                    break;
                }
                else {
                    std::cout << "[NFC] detected(non-ISOA)\n";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
        }

        nfc_close(pnd);
        nfc_exit(ctx);
        return out.ok;
    }

} // namespace sca
