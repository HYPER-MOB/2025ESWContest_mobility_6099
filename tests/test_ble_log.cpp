// tests/test_ble_log.cpp
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <cstdint>
#include <ctime>

static std::string now() {
    char buf[64]; time_t t = time(nullptr);
    std::strftime(buf, sizeof(buf), "%F %T", std::localtime(&t));
    return buf;
}

static bool send_req(int dd, uint16_t ocf, const void* cparam, uint8_t clen, uint8_t* status_out, const char* what) {
    struct hci_request rq {};
    uint8_t status = 0;
    rq.ogf = OGF_LE_CTL;
    rq.ocf = ocf;
    rq.cparam = const_cast<void*>(cparam);
    rq.clen = clen;
    rq.rparam = &status;
    rq.rlen = 1;

    if (hci_send_req(dd, &rq, 1000) < 0) {
        std::fprintf(stderr, "[%s] %s: hci_send_req fail: %s\n", now().c_str(), what, strerror(errno));
        return false;
    }
    if (status != 0x00) {
        std::fprintf(stderr, "[%s] %s: status=0x%02X\n", now().c_str(), what, status);
        if (status_out) *status_out = status;
        return false;
    }
    if (status_out) *status_out = status;
    return true;
}

int main() {
    std::printf("[%s] === BLE TEST START ===\n", now().c_str());

    int dev_id = hci_get_route(nullptr);
    if (dev_id < 0) { std::fprintf(stderr, "no hci device\n"); return 1; }

    int dd = hci_open_dev(dev_id);
    if (dd < 0) { std::fprintf(stderr, "hci_open_dev fail: %s\n", strerror(errno)); return 1; }

    // 0) 광고 끄기(광고가 켜져 있으면 스캔 파라미터 설정이 0x0C로 실패하는 경우가 많음)
    {
        struct le_set_advertise_enable_cp adv {}; adv.enable = 0x00;
        send_req(dd, OCF_LE_SET_ADVERTISE_ENABLE, &adv, LE_SET_ADVERTISE_ENABLE_CP_SIZE, nullptr, "le_adv_enable(OFF)");
    }

    // 1) 스캔 파라미터 설정
    struct le_set_scan_parameters_cp params {};
    params.type = 0x00;                // passive
    params.interval = htobs(0x0010);       // 10ms
    params.window = htobs(0x0010);       // 10ms
    params.own_bdaddr_type = 0x00;                // public
    params.filter = 0x00;                // all
    if (!send_req(dd, OCF_LE_SET_SCAN_PARAMETERS, &params, LE_SET_SCAN_PARAMETERS_CP_SIZE, nullptr, "le_set_scan_params")) {
        // 마무리 후 종료
        hci_close_dev(dd);
        return 1;
    }

    // 2) 스캔 enable
    struct le_set_scan_enable_cp enable {};
    enable.enable = 0x01;
    enable.filter_dup = 0x00;
    if (!send_req(dd, OCF_LE_SET_SCAN_ENABLE, &enable, LE_SET_SCAN_ENABLE_CP_SIZE, nullptr, "le_set_scan_enable(ON)")) {
        hci_close_dev(dd);
        return 1;
    }
    std::fprintf(stderr, "[%s] scan start ok\n", now().c_str());

    // 3) LE 광고 리포트만 수신하도록 필터 설정
    struct hci_filter nf {};
    hci_filter_clear(&nf);
    hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
    hci_filter_set_event(EVT_LE_META_EVENT, &nf);
    if (setsockopt(dd, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
        std::fprintf(stderr, "setsockopt(HCI_FILTER) fail: %s\n", strerror(errno));
        // 그래도 계속 진행
    }

    // 4) 수신 루프 (5초)
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    time_t t0 = time(nullptr);
    while (difftime(time(nullptr), t0) < 5.0) {
        int n = read(dd, buf, sizeof(buf));
        if (n <= 0) continue;

        evt_le_meta_event* meta = (evt_le_meta_event*)(buf + (1 + HCI_EVENT_HDR_SIZE));
        if (meta->subevent != EVT_LE_ADVERTISING_REPORT) continue;

        uint8_t* ptr = meta->data + 1; // num_reports=1 가정
        uint8_t evt_type = ptr[0];
        uint8_t addr_type = ptr[1];
        bdaddr_t addr;
        std::memcpy(&addr, ptr + 2, 6);
        ptr += 8;

        uint8_t len = *ptr++;
        std::string payload;
        char tmp[4];
        for (int i = 0; i < len; ++i) { std::snprintf(tmp, sizeof(tmp), "%02X", ptr[i]); payload += tmp; }
        ptr += len;

        int8_t rssi = (int8_t)*ptr;

        char addrstr[18];
        ba2str(&addr, addrstr);
        std::printf("[%s] ADV %s type=%u addrtype=%u rssi=%d payload=%s\n",
            now().c_str(), addrstr, evt_type, addr_type, rssi, payload.c_str());
    }

    // 5) 스캔 종료
    enable.enable = 0x00; enable.filter_dup = 0x00;
    send_req(dd, OCF_LE_SET_SCAN_ENABLE, &enable, LE_SET_SCAN_ENABLE_CP_SIZE, nullptr, "le_set_scan_enable(OFF)");

    hci_close_dev(dd);
    return 0;
}
