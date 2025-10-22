#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

static int can_open(const char* ifname) {
    int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) return -1;
    struct ifreq ifr {}; std::strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) { close(s); return -1; }
    sockaddr_can addr{}; addr.can_family = AF_CAN; addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(s, (sockaddr*)&addr, sizeof(addr)) < 0) { close(s); return -1; }
    return s;
}

static bool can_send(int s, uint32_t id, uint8_t code) {
    struct can_frame f {};
    f.can_id = id; f.can_dlc = 1; f.data[0] = code;
    return write(s, &f, sizeof(f)) == (ssize_t)sizeof(f);
}

static int run_and_log(const char* bin, const char* logFile) {
    std::string cmd = std::string(bin) + " >> " + logFile + " 2>&1";
    return system(cmd.c_str());
}

int main() {
    const uint32_t ID_REQ_BLE = 0x100;
    const uint32_t ID_REQ_NFC = 0x101;
    const uint32_t ID_RSP_BLE = 0x110;
    const uint32_t ID_RSP_NFC = 0x111;

    system("mkdir -p logs");

    int s = can_open("can0");
    if (s < 0) { perror("can0"); return 1; }
    puts("[router] waiting on can0...");

    while (true) {
        struct can_frame f {};
        if (read(s, &f, sizeof(f)) != (ssize_t)sizeof(f)) continue;

        if (f.can_id == ID_REQ_BLE) {
            puts("[router] BLE_REQ");
            int rc = run_and_log("./test_ble_log", "logs/ble_run.log");
            can_send(s, ID_RSP_BLE, (rc == 0) ? 0x00 : 0xFF);
        }
        else if (f.can_id == ID_REQ_NFC) {
            puts("[router] NFC_REQ");
            int rc = run_and_log("./test_nfc_log", "logs/nfc_run.log");
            can_send(s, ID_RSP_NFC, (rc == 0) ? 0x00 : 0xFF);
        }
    }
}
