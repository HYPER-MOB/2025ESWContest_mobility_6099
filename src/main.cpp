#include <cstdio>
#include <thread>
#include <chrono>

// 네가 가진 C++ 래퍼 헤더
#include "can_api.hpp"

#include "sequencer.hpp"

// CAN 콜백: 모든 프레임을 sequencer로 전달
static Sequencer* g_seq = nullptr;

// 기존 can_api.hpp의 콜백 시그니처에 맞춰 작성
static void on_rx_cb(const CanFrame* f, void* user) {
    (void)user;
    if (g_seq && f) g_seq->on_can_rx(*f);
}

int main() {
    // ===== CAN 초기화/오픈 =====
    if (can_init(CAN_DEVICE_DEBUG) != CAN_OK) {
        std::fprintf(stderr, "can_init failed\n");
        return 1;
    }
    CanConfig cfg { .channel=0, .bitrate=500000, .samplePoint=0.875f, .sjw=1, .mode=CAN_MODE_NORMAL };
    const char* CH = "debug0";
    if (can_open(CH, cfg) != CAN_OK) {
        std::fprintf(stderr, "can_open failed\n");
        return 1;
    }

    // 모든 프레임 구독(마스크 전체 0)
    CanFilter any = { .type = CAN_FILTER_MASK };
    any.data.mask.id = 0; any.data.mask.mask = 0;
    if (can_subscribe(CH, any, on_rx_cb, nullptr) <= 0) {
        std::fprintf(stderr, "can_subscribe failed\n");
        return 1;
    }

    // ===== Sequencer 구성 =====
    SequencerConfig scfg;
    scfg.can_channel     = CH;
    scfg.can_bitrate     = 500000;
    scfg.ble_local_name  = "SCA-CAR";
    scfg.ble_timeout_s   = 30;
    scfg.nfc_timeout_s   = 5;
    scfg.ble_uuid_last12 = "A1B2C3D4E5F6"; // 필요시 정책에 맞게 변경

    Sequencer seq(scfg);
    g_seq = &seq;

    std::puts("[main] Waiting for DCU_SCA_USER_FACE_REQ(0x101) ...");

    // 메인 루프: tick() 호출로 상태 진행
    while (true) {
        seq.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 20Hz
    }

    // can_dispose(); // 도달하지 않음
    // return 0;
}
