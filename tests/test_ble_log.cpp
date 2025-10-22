#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <unistd.h>
#include <sys/time.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <cstdint>
#include <ctime>

static std::string now() {
    char buf[64]; std::time_t t = std::time(nullptr);
    std::strftime(buf, sizeof(buf), "%F %T", std::localtime(&t));
    return buf;
}

int main() {
    const char* LOG = "logs/ble_run.log";
    std::FILE* lf = std::fopen(LOG, "a");
    if (!lf) { perror("open log"); return 1; }
    std::fprintf(lf, "[%s] === BLE TEST START ===\n", now().c_str()); std::fflush(lf);

    // 기본 HCI 디바이스 선택
    int dev_id = hci_get_route(nullptr);
    if (dev_id < 0) { std::fprintf(lf, "no hci device\n"); std::fclose(lf); return 2; }

    // BlueZ의 high-level API 사용 (구조체 직접 X)
    // interval/window: 0x0010(10ms 근처), passive 스캔
    if (hci_le_set_scan_parameters(dev_id,
        0x00,        /* type: passive */
        htobs(0x0010), /* interval */
        htobs(0x0010), /* window   */
        0x00,        /* own addr type: public */
        0x00,        /* filter policy: none */
        1000) < 0)   /* timeout ms for mgmt op */
    {
        std::fprintf(lf, "set scan params fail\n"); std::fclose(lf); return 3;
    }

    if (hci_le_set_scan_enable(dev_id, 0x01, 0x00, 1000) < 0) {
        std::fprintf(lf, "enable scan fail\n"); std::fclose(lf); return 4;
    }

    // raw 소켓으로 이벤트 수집
    int dd = hci_open_dev(dev_id);
    if (dd < 0) { std::fprintf(lf, "open hci failed\n"); std::fclose(lf); return 5; }

    uint8_t buf[HCI_MAX_EVENT_SIZE];
    int found = 0;
    timeval end_tv; gettimeofday(&end_tv, nullptr); end_tv.tv_sec += 5;

    while (true) {
        timeval now_tv; gettimeofday(&now_tv, nullptr);
        if (timercmp(&now_tv, &end_tv, > )) break;
        int n = ::read(dd, buf, sizeof(buf));
        if (n > 0) found++;
    }

    hci_close_dev(dd);
    hci_le_set_scan_enable(dev_id, 0x00, 0x00, 1000); // 스캔 중지

    std::fprintf(lf, "[%s] BLE adv seen: %d\n", now().c_str(), found);
    std::fprintf(lf, "[%s] === BLE TEST END ===\n\n", now().c_str());
    std::fclose(lf);
    return 0;
}
