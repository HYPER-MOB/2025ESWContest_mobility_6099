// 간단 NFC 테스트 (libnfc) — 태그 감지 시 UID 로그 출력
// 빌드 전제: libnfc-dev, NFC 리더 연결(예: PN532)
#include <iostream>
#include <vector>
#include <chrono>
#include <csignal>

extern "C" {
#include <nfc/nfc.h>
}

static bool g_stop = false;
static void on_sigint(int) { g_stop = true; }

// 바이트 → 헥스 문자열
static std::string hex(const uint8_t* d, size_t n) {
    static const char* k = "0123456789ABCDEF";
    std::string s; s.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) { s.push_back(k[d[i] >> 4]); s.push_back(k[d[i] & 0xF]); }
    return s;
}

int main(int argc, char** argv) {
    std::signal(SIGINT, on_sigint);
    std::signal(SIGTERM, on_sigint);

    int seconds = (argc > 1 ? std::stoi(argv[1]) : 5); // 기본 5초

    nfc_context* ctx = nullptr;
    nfc_init(&ctx);
    if (!ctx) { std::cerr << "[ERR] nfc_init failed\n"; return 1; }

    nfc_device* pnd = nfc_open(ctx, nullptr); // default
    if (!pnd) { std::cerr << "[ERR] nfc_open failed\n"; nfc_exit(ctx); return 1; }

    if (nfc_initiator_init(pnd) < 0) {
        std::cerr << "[ERR] nfc_initiator_init failed\n";
        nfc_close(pnd); nfc_exit(ctx); return 1;
    }

    std::cout << "[RUN] NFC polling for " << seconds << "s ..." << std::endl;

    // 다양한 모듈레이션 시도 (필요시 조정)
    const nfc_modulation mods[] = {
      { NMT_ISO14443A, NBR_106 },
      { NMT_FELICA,    NBR_212 },
      { NMT_FELICA,    NBR_424 },
      { NMT_ISO14443B, NBR_106 },
      { NMT_JEWEL,     NBR_106 },
    };

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    while (!g_stop && std::chrono::steady_clock::now() < deadline) {
        nfc_target nt{};
        int res = nfc_initiator_poll_target(pnd, mods, sizeof(mods) / sizeof(mods[0]),
            2, /* 각 모드 타임슬롯 */
            200); /* ms 타임아웃 */
        if (res > 0) {
            // ISO14443A 기준의 UID 출력 (다른 타입이면 필드 다름)
            if (nt.nti.nai.szUidLen) {
                std::cout << "[NFC] UID=" << hex(nt.nti.nai.abtUid, nt.nti.nai.szUidLen) << std::endl;
            }
            else {
                std::cout << "[NFC] target detected (non-ISOA)" << std::endl;
            }
            // 한 번 본 뒤 살짝 쉬기
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    }

    nfc_close(pnd);
    nfc_exit(ctx);
    std::cout << "[DONE] NFC polling finished" << std::endl;
    return 0;
}
