// 간단 BLE 스캐너 (BlueZ HCI Raw) — 결과를 표준출력으로 로그(스크립트가 파일로 저장)
// 빌드 전제: libbluetooth-dev / bluez
#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>
#include <csignal>
#include <vector>

extern "C" {
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
}

static bool g_stop = false;
static void on_sigint(int) { g_stop = true; }

// MAC을 시:분:… 형태 문자열로 변환
static std::string mac_to_string(const bdaddr_t& a) {
    char addr[18]{};
    ba2str(&a, addr);
    return std::string(addr);
}

// AD(EIR)에서 이름 추출 (0x09 Complete Local Name or 0x08 Shortened)
static std::string parse_name(const uint8_t* data, size_t len) {
    size_t idx = 0;
    while (idx + 2 <= len) {
        uint8_t field_len = data[idx];
        if (field_len == 0) break;
        if (idx + 1 + field_len > len) break;
        uint8_t type = data[idx + 1];
        const uint8_t* val = &data[idx + 2];
        size_t vlen = field_len - 1;

        if (type == 0x09 || type == 0x08) {
            return std::string(reinterpret_cast<const char*>(val), vlen);
        }
        idx += (1 + field_len);
    }
    return "";
}

int main(int argc, char** argv) {
    std::signal(SIGINT, on_sigint);
    std::signal(SIGTERM, on_sigint);

    int seconds = (argc > 1 ? std::stoi(argv[1]) : 5); // 기본 5초 스캔

    // HCI 디바이스 선택/오픈
    int dev_id = hci_get_route(nullptr);
    if (dev_id < 0) { std::cerr << "[ERR] no HCI device\n"; return 1; }
    int dd = hci_open_dev(dev_id);
    if (dd < 0) { std::cerr << "[ERR] hci_open_dev failed\n"; return 1; }

    // 스캔 파라미터 설정 (LE)
    le_set_scan_parameters_cp scan_params{};
    scan_params.type = 0x00;       // Passive
    scan_params.interval = htobs(0x0010);
    scan_params.window = htobs(0x0010);
    scan_params.own_bdaddr_type = 0x00; // public
    scan_params.filter = 0x00;     // accept all
    if (hci_send_cmd(dd, OGF_LE_CTL, OCF_LE_SET_SCAN_PARAMETERS,
        LE_SET_SCAN_PARAMETERS_CP_SIZE, &scan_params) < 0) {
        std::cerr << "[ERR] set_scan_params\n"; hci_close_dev(dd); return 1;
    }

    // 스캔 시작
    le_set_scan_enable_cp enable_cp{};
    enable_cp.enable = 0x01;     // enable
    enable_cp.filter_dup = 0x01; // filter duplicates
    if (hci_send_cmd(dd, OGF_LE_CTL, OCF_LE_SET_SCAN_ENABLE,
        LE_SET_SCAN_ENABLE_CP_SIZE, &enable_cp) < 0) {
        std::cerr << "[ERR] set_scan_enable\n"; hci_close_dev(dd); return 1;
    }

    std::cout << "[RUN] BLE scanning for " << seconds << "s ..." << std::endl;

    // 수신 필터: LE Meta Event만
    struct hci_filter nf {};
    hci_filter_clear(&nf);
    hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
    hci_filter_set_event(EVT_LE_META_EVENT, &nf);
    if (setsockopt(dd, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
        std::cerr << "[WARN] setsockopt HCI_FILTER failed\n";
    }

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    while (!g_stop && std::chrono::steady_clock::now() < deadline) {
        uint8_t buf[HCI_MAX_EVENT_SIZE];
        int len = read(dd, buf, sizeof(buf));
        if (len <= 0) continue;

        evt_le_meta_event* meta = (evt_le_meta_event*)(buf + (1 + HCI_EVENT_HDR_SIZE));
        if (meta->subevent != EVT_LE_ADVERTISING_REPORT) continue;

        uint8_t* ptr = meta->data + 1; // [0]=num reports
        uint8_t num_reports = meta->data[0];
        for (uint8_t i = 0; i < num_reports; ++i) {
            // 형식: evt_le_advertising_info
            uint8_t evt_type = ptr[0];
            bdaddr_t bdaddr; memcpy(&bdaddr, ptr + 1, 6);
            ptr += 7;
            uint8_t data_len = ptr[0]; ptr += 1;
            std::string name = parse_name(ptr, data_len);
            ptr += data_len;
            int8_t rssi = (int8_t)*ptr; ptr += 1;

            std::cout << "[BLE] mac=" << mac_to_string(bdaddr)
                << " rssi=" << (int)rssi
                << (name.empty() ? "" : (" name=\"" + name + "\""))
                << std::endl;
        }
    }

    // 스캔 중지
    enable_cp.enable = 0x00;
    enable_cp.filter_dup = 0x00;
    hci_send_cmd(dd, OGF_LE_CTL, OCF_LE_SET_SCAN_ENABLE,
        LE_SET_SCAN_ENABLE_CP_SIZE, &enable_cp);

    hci_close_dev(dd);
    std::cout << "[DONE] BLE scan finished" << std::endl;
    return 0;
}
