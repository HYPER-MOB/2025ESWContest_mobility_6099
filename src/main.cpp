#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <thread>
#include <chrono>
#include <atomic>

#include "can_api.hpp"
#include "can_ids.hpp"
#include "sequencer.hpp"

// can_api�� ���� ������ ���ǿ� ���� ����
struct CanFrame {
    uint32_t id = 0;
    uint8_t  dlc = 0;
    uint8_t  data[8] = { 0 };
};

// ���� �ݹ�
static Sequencer* g_seq = nullptr;

static void on_rx_cb(const CanFrame* f, void* /*user*/) {
    if (!g_seq) return;
    g_seq->on_can_rx(*f);
}

int main() {
    const char* CH = "debug0";  // ���� �����: "can0"�� �ٲ㵵 ��

    // CAN �ʱ�ȭ/����
    if (can_init(CAN_DEVICE_DEBUG) != CAN_OK) {
        std::fprintf(stderr, "can_init failed\n");
        return 1;
    }
    CanConfig cfg = { .channel = 0, .bitrate = 500000, .samplePoint = 0.875f, .sjw = 1, .mode = CAN_MODE_NORMAL };
    if (can_open(CH, cfg) != CAN_OK) {
        std::fprintf(stderr, "can_open failed\n");
        return 1;
    }

    // ��� ������ ���� (����=��ü)
    CanFilter any = { .type = CAN_FILTER_MASK };
    any.data.mask.id = 0; any.data.mask.mask = 0;
    if (can_subscribe(CH, any, (can_rx_cb_t)on_rx_cb, nullptr) <= 0) {
        std::fprintf(stderr, "can_subscribe failed\n");
        return 1;
    }

    // ������
    SequencerConfig scfg;
    scfg.can_channel = CH;
    scfg.can_bitrate = 500000;
    scfg.ble_local_name = "SCA-CAR";
    scfg.ble_timeout_s = 30;
    scfg.nfc_timeout_s = 5;
    scfg.ble_token_fallback = "ACCESS";

    Sequencer seq(scfg);
    g_seq = &seq;

    std::puts("[router] waiting for DCU_SCA_USER_FACE_REQ(0x101) ...");

    // ���� ����: ���� �� ���� + ������ tick (���ŷ ����)
    while (true) {
        seq.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // ���� �� ��
    // can_dispose();
    // return 0;
}
