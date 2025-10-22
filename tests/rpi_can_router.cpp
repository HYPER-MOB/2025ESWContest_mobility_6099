// CAN 라우터 (요청 수신→BLE/NFC 트리거→응답)
// - 요청 ID:
//   0x101 : BLE 테스트 트리거 (3초 스캔 시도)
//   0x102 : NFC 테스트 트리거 (3초 폴링 시도)
// - 응답 ID:
//   0x112 : 결과 플래그 1바이트 (0x00=OK, 0x01=NG)
// 빌드 전제: bluez, libnfc, can-utils(실행용)

#include <iostream>
#include <csignal>
#include <chrono>
#include <thread>
#include <vector>
#include <cstring>

extern "C" {
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <nfc/nfc.h>
}

static bool g_stop = false;
static void on_sigint(int) { g_stop = true; }

// 헥스 문자열
static std::string hex(const uint8_t* d, size_t n) {
    static const char* k = "0123456789ABCDEF";
    std::string s; s.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) { s.push_back(k[d[i] >> 4]); s.push_back(k[d[i] & 0xF]); }
    return s;
}

/* ===== BLE 3초 데모 스캔 (성공 여부만 반환) ===== */
static bool demo_ble_scan_3s() {
    int dev_id = hci_get_route(nullptr);
    if (dev_id < 0) return false;
    int dd = hci_open_dev(dev_id);
    if (dd < 0) return false;

    le_set_scan_parameters_cp scan_params{};
    scan_params.type = 0x00;
    scan_params.interval = htobs(0x0010);
    scan_params.window = htobs(0x0010);
    scan_params.own_bdaddr_type = 0x00;
    scan_params.filter = 0x00;
    if (hci_send_cmd(dd, OGF_LE_CTL, OCF_LE_SET_SCAN_PARAMETERS,
        LE_SET_SCAN_PARAMETERS_CP_SIZE, &scan_params) < 0) {
        hci_close_dev(dd); return false;
    }

    le_set_scan_enable_cp enable_cp{};
    enable_cp.enable = 0x01;
    enable_cp.filter_dup = 0x01;
    if (hci_send_cmd(dd, OGF_LE_CTL, OCF_LE_SET_SCAN_ENABLE,
        LE_SET_SCAN_ENABLE_CP_SIZE, &enable_cp) < 0) {
        hci_close_dev(dd); return false;
    }

    // 간단히 2~3개 이벤트만 받으면 성공으로 간주
    bool ok = false; int hits = 0;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (std::chrono::steady_clock::now() < deadline) {
        uint8_t buf[HCI_MAX_EVENT_SIZE];
        int len = read(dd, buf, sizeof(buf));
        if (len <= 0) continue;
        // 대충 LE 메타가 보이면 hit
        if (buf[1] == EVT_LE_META_EVENT) {
            if (++hits >= 1) { ok = true; break; }
        }
    }

    enable_cp.enable = 0x00; enable_cp.filter_dup = 0x00;
    hci_send_cmd(dd, OGF_LE_CTL, OCF_LE_SET_SCAN_ENABLE,
        LE_SET_SCAN_ENABLE_CP_SIZE, &enable_cp);
    hci_close_dev(dd);
    return ok;
}

/* ===== NFC 3초 데모 폴링 (성공 여부만 반환) ===== */
static bool demo_nfc_poll_3s() {
    nfc_context* ctx = nullptr; nfc_init(&ctx);
    if (!ctx) return false;
    nfc_device* pnd = nfc_open(ctx, nullptr);
    if (!pnd) { nfc_exit(ctx); return false; }
    if (nfc_initiator_init(pnd) < 0) { nfc_close(pnd); nfc_exit(ctx); return false; }

    const nfc_modulation mod{ NMT_ISO14443A, NBR_106 };
    bool ok = false;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (std::chrono::steady_clock::now() < deadline) {
        nfc_target nt{};
        int r = nfc_initiator_select_passive_target(pnd, mod, nullptr, 0, &nt);
        if (r > 0) { ok = true; break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    nfc_close(pnd);
    nfc_exit(ctx);
    return ok;
}

/* ===== CAN 래퍼 ===== */
static int can_open(const std::string& ifname) {
    int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) return -1;
    struct ifreq ifr {};
    std::strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ - 1);
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) { close(s); return -1; }
    sockaddr_can addr{}; addr.can_family = AF_CAN; addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) { close(s); return -1; }

    // 수신필터: 0x101, 0x102
    can_filter flt[2]{};
    flt[0].can_id = 0x101; flt[0].can_mask = CAN_SFF_MASK;
    flt[1].can_id = 0x102; flt[1].can_mask = CAN_SFF_MASK;
    setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, &flt, sizeof(flt));
    return s;
}

static bool can_send_flag(int s, uint32_t id, uint8_t flag) {
    struct can_frame f {};
    f.can_id = id;
    f.can_dlc = 1;
    f.data[0] = flag;
    return write(s, &f, sizeof(f)) == (ssize_t)sizeof(f);
}

int main(int argc, char** argv) {
    std::signal(SIGINT, on_sigint);
    std::signal(SIGTERM, on_sigint);

    std::string ifname = (argc > 1 ? argv[1] : "can0");
    int s = can_open(ifname);
    if (s < 0) { std::cerr << "[ERR] CAN open fail: " << ifname << "\n"; return 1; }
    std::cout << "[RUN] CAN router on " << ifname
        << " (RX 0x101=BLE, 0x102=NFC; TX 0x112 flag)\n";

    while (!g_stop) {
        struct can_frame f {};
        int n = read(s, &f, sizeof(f));
        if (n != (int)sizeof(f)) continue;

        std::cout << "[RX] id=0x" << std::hex << std::uppercase << f.can_id
            << " dlc=" << std::dec << (int)f.can_dlc
            << " data=" << hex(f.data, f.can_dlc) << std::endl;

        uint8_t flag = 0x00; // 0=성공, 1=실패
        if (f.can_id == 0x101) {
            bool ok = demo_ble_scan_3s();
            flag = ok ? 0x00 : 0x01;
            can_send_flag(s, 0x112, flag);
            std::cout << "[TX] id=0x112 flag=" << (int)flag << std::endl;
        }
        else if (f.can_id == 0x102) {
            bool ok = demo_nfc_poll_3s();
            flag = ok ? 0x00 : 0x01;
            can_send_flag(s, 0x112, flag);
            std::cout << "[TX] id=0x112 flag=" << (int)flag << std::endl;
        }
    }

    close(s);
    std::cout << "[BYE] rpi_can_router exit\n";
    return 0;
}
