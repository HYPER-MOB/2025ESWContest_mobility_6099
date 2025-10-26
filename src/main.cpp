#include <cstdio>
#include <thread>
#include <chrono>

// �װ� ���� C++ ���� ���
#include "can_api.hpp"

#include "sequencer.hpp"

// CAN �ݹ�: ��� �������� sequencer�� ����
static Sequencer* g_seq = nullptr;

static int run_cmd(const char* cmd) {
    int rc = std::system(cmd);
    if (rc != 0) std::fprintf(stderr, "[can0] cmd failed (%d): %s\n", rc, cmd);
    return rc;
}

bool bringup_can0(unsigned bitrate = 500000, bool canfd = false) {
    // 내려두고 시작(에러 무시)
    run_cmd("ip link set can0 down 2>/dev/null || true");

    char buf[256];
    if (canfd) {
        std::snprintf(buf, sizeof(buf),
            "ip link set can0 type can bitrate %u dbitrate 2000000 fd on", bitrate);
    } else {
        std::snprintf(buf, sizeof(buf),
            "ip link set can0 type can bitrate %u", bitrate);
    }
    if (run_cmd(buf) != 0) return false;

    run_cmd("ip link set can0 txqueuelen 1024");

    if (run_cmd("ip link set can0 up") != 0) return false;

    return true;
}

void bringdown_can0() {
    run_cmd("ip link set can0 down 2>/dev/null || true");
}
// ���� can_api.hpp�� �ݹ� �ñ״�ó�� ���� �ۼ�
static void on_rx_cb(const CanFrame* f, void* user) {
    (void)user;
    if (g_seq && f) g_seq->on_can_rx(*f);
}

int main() {
    
    if (!bringup_can0(500000, /*canfd=*/false)) {
    std::fprintf(stderr, "[can0] bringup failed\n");
    // 필요시 종료 또는 재시도
    }
    
    // ===== CAN �ʱ�ȭ/���� =====
    if (can_init(CAN_DEVICE_LINUX) != CAN_OK) {
        std::fprintf(stderr, "can_init failed\n");
        return 1;
    }

    CanConfig cfg { .channel=0, .bitrate=500000, .samplePoint=0.875f, .sjw=1, .mode=CAN_MODE_NORMAL };
    const char* CH = "can0";
    if (can_open(CH, cfg) != CAN_OK) {
        std::fprintf(stderr, "can_open failed\n");
        return 1;
    }

    // ===== Sequencer ���� =====
    SequencerConfig scfg;
    scfg.can_channel     = CH;
    scfg.can_bitrate     = 500000;
    scfg.ble_local_name  = "SCA-CAR";
    scfg.ble_timeout_s   = 30;
    scfg.nfc_timeout_s   = 5;
    scfg.ble_uuid_last12 = "A1B2C3D4E5F6"; // �ʿ�� ��å�� �°� ����

    Sequencer seq(scfg);
    g_seq = &seq;


    // ��� ������ ����(����ũ ��ü 0)
    CanFilter any = { .type = CAN_FILTER_MASK };
    any.data.mask.id = 0; any.data.mask.mask = 0;
    if (can_subscribe(CH, any, on_rx_cb, nullptr) <= 0) {
        std::fprintf(stderr, "can_subscribe failed\n");
        return 1;
    }
    std::puts("[main] Waiting for DCU_SCA_USER_FACE_REQ(0x101) ...");
    // ���� ����: tick() ȣ��� ���� ����
    while (true) {
        seq.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 20Hz
    }

    // can_dispose(); // �������� ����
    // return 0;
}
