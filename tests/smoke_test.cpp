// ���� ���� CAN ����ũ �׽�Ʈ(C++):
// 1) can_api �ʱ�ȭ �� ä�� ����
// 2) ���� �ݹ� ���(��� ������ ���)
// 3) 500ms���� ������ �ϳ� �۽�
// 4) Ctrl+C�� ����

#include <csignal>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <atomic>
#include <chrono>

#include "can_api.hpp"       // C++�� ��ȯ�� can_core ���
#include "canmessage.hpp"

static std::atomic_bool g_stop{ false };

static void on_sigint(int) { g_stop.store(true); }

static void on_rx(const CanFrame* f, void* /*user*/) {
    std::printf("[RX] id=0x%X dlc=%u :", (unsigned)f->id, f->dlc);
    for (uint8_t i = 0; i < f->dlc && i < 8; i++) std::printf(" %02X", f->data[i]);
    std::puts("");
}

int main() {
    std::signal(SIGINT, on_sigint);
    std::signal(SIGTERM, on_sigint);

    const char* CH = "debug0";

    // �鿣�忡 �°� ����(CAN_DEVICE_DEBUG / CAN_DEVICE_LINUX)
    if (can_init(CAN_DEVICE_DEBUG) != CAN_OK) {  // �ǹ����� CAN_DEVICE_LINUX
        std::fprintf(stderr, "can_init failed\n");
        return 1;
    }

    CanConfig cfg{};
    cfg.channel = 0;
    cfg.bitrate = 500000;
    cfg.samplePoint = 0.875f;
    cfg.sjw = 1;
    cfg.mode = CAN_MODE_LOOPBACK;        // �ǹ����� CAN_MODE_NORMAL

    if (can_open(CH, cfg) != CAN_OK) {
        std::fprintf(stderr, "can_open failed\n");
        return 1;
    }

    // ��� ������ ����
    CanFilter any{};
    any.type = CAN_FILTER_MASK;
    any.data.mask.id = 0x000;
    any.data.mask.mask = 0x000;
    if (can_subscribe(CH, any, on_rx, nullptr) <= 0) {
        std::fprintf(stderr, "can_subscribe failed\n");
        return 1;
    }

    std::puts("[smoke] start: send 0x123 every 500ms (Ctrl+C to stop)");

    uint8_t counter = 0;
    while (!g_stop.load()) {
        CanFrame tx{};
        tx.id = 0x123;
        tx.dlc = 4;
        tx.data[0] = 0xDE;
        tx.data[1] = 0xAD;
        tx.data[2] = 0xBE;
        tx.data[3] = counter++;

        can_err_t r = can_send(CH, tx, 0);
        if (r != CAN_OK) {
            std::fprintf(stderr, "send err=%d\n", r);
        }
        else {
            std::printf("[TX] id=0x%X dlc=%u : %02X %02X %02X %02X\n",
                (unsigned)tx.id, tx.dlc, tx.data[0], tx.data[1], tx.data[2], tx.data[3]);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    can_dispose();
    std::puts("[smoke] bye");
    return 0;
}
