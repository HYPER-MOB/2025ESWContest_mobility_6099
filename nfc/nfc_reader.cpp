#include "nfc_reader.hpp"
#include <array>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <string>
#include <vector>
#include <iostream>

extern "C" {
#include <nfc/nfc.h>
}

using namespace std::chrono;

static std::string hex_upper(const uint8_t* d, size_t n) {
    static const char* k = "0123456789ABCDEF";
    std::string s; s.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) { s.push_back(k[d[i] >> 4]); s.push_back(k[d[i] & 0xF]); }
    return s;
}

bool read_nfc_uid(std::string& out_uid, int timeout_sec)
{
#ifdef USE_LIBNFC
    out_uid.clear();
    nfc_context* ctx = nullptr; nfc_init(&ctx);
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
    const size_t mods_count = sizeof(mods) / sizeof(mods[0]);
    const uint8_t uiPollNr = 1, uiPeriod = 2; // ~300ms

    auto deadline = steady_clock::now() + seconds(timeout_sec);
    while (steady_clock::now() < deadline) {
        nfc_target nt{};
        int res = nfc_initiator_poll_target(pnd, mods, mods_count, uiPollNr, uiPeriod, &nt);
        if (res > 0) {
            if (nt.nm.nmt == NMT_ISO14443A && nt.nti.nai.szUidLen > 0) {
                out_uid = hex_upper(nt.nti.nai.abtUid, nt.nti.nai.szUidLen);
                nfc_close(pnd); nfc_exit(ctx); return true;
            }
            if (nt.nm.nmt == NMT_ISO14443B) {
                out_uid = hex_upper(nt.nti.nbi.abtPupi, 4);
                nfc_close(pnd); nfc_exit(ctx); return true;
            }
            if (nt.nm.nmt == NMT_FELICA) {
                out_uid = hex_upper(nt.nti.nfi.abtId, 8);
                nfc_close(pnd); nfc_exit(ctx); return true;
            }
            if (nt.nm.nmt == NMT_JEWEL) {
                out_uid = hex_upper(nt.nti.nji.btId, 4);
                nfc_close(pnd); nfc_exit(ctx); return true;
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(120));
        }
    }
    nfc_close(pnd); nfc_exit(ctx);
    return false;
#else
    (void)out_uid; (void)timeout_sec; return false;
#endif
}
