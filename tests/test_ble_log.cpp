#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <unistd.h>
#include <sys/time.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static std::string now() {
    char buf[64]; time_t t = time(nullptr);
    strftime(buf, sizeof(buf), "%F %T", localtime(&t)); return buf;
}

int main() {
    const char* LOG = "logs/ble_run.log";
    FILE* lf = fopen(LOG, "a");
    if (!lf) { perror("open log"); return 1; }
    fprintf(lf, "[%s] === BLE TEST START ===\n", now().c_str()); fflush(lf);

    int dev_id = hci_get_route(nullptr);
    if (dev_id < 0) { fprintf(lf, "no hci device\n"); fclose(lf); return 2; }

    int dd = hci_open_dev(dev_id);
    if (dd < 0) { fprintf(lf, "open hci failed\n"); fclose(lf); return 3; }

    // 파라미터: passive scan, 빠른 주기
    le_set_scan_parameters_cp params{};
    params.type = 0x00; // passive
    params.interval = htobs(0x0010);
    params.window = htobs(0x0010);
    params.own_bdaddr_type = 0x00;
    params.filter = 0x00;
    if (hci_send_cmd(dd, OGF_LE_CTL, OCF_LE_SET_SCAN_PARAMETERS,
        LE_SET_SCAN_PARAMETERS_CP_SIZE, &params) < 0) {
        fprintf(lf, "set scan params fail\n"); fclose(lf); hci_close_dev(dd); return 4;
    }

    le_set_scan_enable_cp enable{};
    enable.enable = 0x01; enable.filter_dup = 0x00;
    if (hci_send_cmd(dd, OGF_LE_CTL, OCF_LE_SET_SCAN_ENABLE,
        LE_SET_SCAN_ENABLE_CP_SIZE, &enable) < 0) {
        fprintf(lf, "enable scan fail\n"); fclose(lf); hci_close_dev(dd); return 5;
    }

    // 5초간 이벤트 읽기
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    int found = 0;
    struct timeval tv_end; gettimeofday(&tv_end, nullptr); tv_end.tv_sec += 5;

    while (true) {
        struct timeval tv_now; gettimeofday(&tv_now, nullptr);
        if (timercmp(&tv_now, &tv_end, > )) break;

        int n = read(dd, buf, sizeof(buf));
        if (n <= 0) continue;
        // 간단히 카운트만
        found++;
    }

    // 스캔 중지
    enable.enable = 0x00;
    hci_send_cmd(dd, OGF_LE_CTL, OCF_LE_SET_SCAN_ENABLE,
        LE_SET_SCAN_ENABLE_CP_SIZE, &enable);
    hci_close_dev(dd);

    fprintf(lf, "[%s] BLE adv seen: %d\n", now().c_str(), found);
    fprintf(lf, "[%s] === BLE TEST END ===\n\n", now().c_str());
    fclose(lf);

    // 성공 기준: 최소 0개여도 에러 아님(환경 따라 다름)
    return 0;
}
