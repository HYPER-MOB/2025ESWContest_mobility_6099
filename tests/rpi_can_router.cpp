// g++ -std=c++17 router.cpp -o router
// ����: sudo ./router
//
// ���� ���
// - CAN ID_REQ_BLE(0x100) ���� �� ./sca_ble_peripheral ���� �� ACCESS ���� �����̸� ID_RSP_BLE(0x110, 0x00) �۽�, �ƴϸ� 0xFF
// - CAN ID_REQ_NFC(0x101) ���� �� ./test_nfc_log ���� �� UID �Ľ� �� ��� UID�� �� �� ID_RSP_NFC(0x111, 0x00/0xFF) �۽�
//
// �غ�
// - ���� ������ sca_ble_peripheral, test_nfc_log ���̳ʸ� ����
// - SocketCAN: sudo ip link set can0 up type can bitrate 500000

#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <net/if.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

static constexpr uint32_t ID_REQ_BLE = 0x100;
static constexpr uint32_t ID_REQ_NFC = 0x101;
static constexpr uint32_t ID_RSP_BLE = 0x110;
static constexpr uint32_t ID_RSP_NFC = 0x111;

// ====== ����(�ʿ�� ����) ======
static std::string DEFAULT_HASH = "A1B2C3D4E5F60708"; // 16-hex(8����Ʈ)
static std::string ADV_NAME = "SCA-CAR";
static int         BLE_TIMEOUT = 60;

// �� ��� NFC UID (�빮��/������� 16�� ���ڿ��� �����ּ���)
static std::string EXPECTED_NFC_UID = "04AABBCCDDEEFF"; // ��: "04AABBCCDDEEFF"

// ====== CAN helpers ======
static int can_open(const char* ifname) {
    int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) { perror("socket"); return -1; }
    struct ifreq ifr {}; std::strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) { perror("ioctl"); ::close(s); return -1; }
    sockaddr_can addr{}; addr.can_family = AF_CAN; addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(s, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); ::close(s); return -1; }
    return s;
}
static bool can_send_byte(int s, uint32_t id, uint8_t code) {
    struct can_frame f {}; f.can_id = id; f.can_dlc = 1; f.data[0] = code;
    return ::write(s, &f, sizeof(f)) == (ssize_t)sizeof(f);
}

// ====== ����: ���� ���μ��� ���� & ��� ĸó ======
static int run_and_capture_lines(const std::string& cmd,
    const std::string& logfile,
    std::vector<std::string>& out_lines) {
    system("mkdir -p logs");
    std::ofstream log(logfile, std::ios::app);

    std::cout << "[router] spawn: " << cmd << std::endl;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "[router] popen failed\n";
        return 1;
    }
    char line[1024];
    while (fgets(line, sizeof(line), pipe)) {
        std::string s(line);
        if (log.is_open()) log << s;
        std::cout << s;
        out_lines.push_back(s);
    }
    int status = pclose(pipe);
    int exitcode = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    if (log.is_open()) {
        log << "[router] proc exit=" << exitcode << "\n";
        log.flush();
    }
    return exitcode;
}

// ====== BLE: sca_ble_peripheral ���� & Write ������ ���� ======
static int run_ble_and_capture(const std::string& hash16,
    const std::string& adv_name,
    int timeout_sec,
    std::string& out_written_data)
{
    char cmd[512];
    std::snprintf(cmd, sizeof(cmd),
        "./sca_ble_peripheral --hash %s --name %s --timeout %d",
        hash16.c_str(), adv_name.c_str(), timeout_sec);

    std::vector<std::string> lines;
    int rc = run_and_capture_lines(cmd, "logs/ble_run.log", lines);

    // ��¿��� data=�� ����(������ ���)
    for (auto& s : lines) {
        auto pos = s.find("data=");
        if (pos != std::string::npos) {
            out_written_data = s.substr(pos + 5);
            while (!out_written_data.empty() &&
                (out_written_data.back() == '\n' || out_written_data.back() == '\r'))
                out_written_data.pop_back();
        }
    }
    std::cout << "[router] BLE captured=\"" << out_written_data << "\" exit=" << rc << "\n";
    return rc; // 0=ACCESS ����, �� �� ����
}

// ====== NFC: test_nfc_log ���� & UID �Ľ�/���� ======
static bool parse_uid_from_line(const std::string& s, std::string& uid_out) {
    // ����: "[NFC] UID=04AABBCCDDEEFF" �Ǵ� "UID: 04 AA BB ..."
    auto p = s.find("UID");
    if (p == std::string::npos) return false;

    // '=' �Ǵ� ':' ������ ��ū���� �ٿ��� 16���� ����
    auto q = s.find_first_of("=:", p);
    if (q == std::string::npos) return false;
    std::string t = s.substr(q + 1);

    std::string hex;
    for (char c : t) {
        if (('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')) hex.push_back(toupper(c));
    }
    if (hex.size() >= 6) { uid_out = hex; return true; } // �ּ� �� ����Ʈ��
    return false;
}

static int run_nfc_and_judge(bool& out_match, std::string& out_uid) {
    std::vector<std::string> lines;
    int rc = run_and_capture_lines("./test_nfc_log", "logs/nfc_run.log", lines);

    // ��¿��� UID �� �� ã�ƺ���
    std::string uid;
    for (auto& s : lines) {
        if (parse_uid_from_line(s, uid)) break;
    }
    out_uid = uid;

    // rc ��ü�� 0�� �ƴϴ���, UID�� �������� �װɷ� ���� ����
    if (!uid.empty()) {
        out_match = (uid == EXPECTED_NFC_UID);
    }
    else {
        out_match = false; // UID �� ������� ����
    }

    std::cout << "[router] NFC uid=\"" << uid << "\" expected=\"" << EXPECTED_NFC_UID
        << "\" match=" << (out_match ? "true" : "false") << " exit=" << rc << "\n";
    return (out_match ? 0 : 1);
}

// ====== main ======
int main() {
    int s = can_open("can0");
    if (s < 0) return 1;
    std::cout << "[router] waiting on can0..." << std::endl;

    while (true) {
        struct can_frame f {};
        ssize_t n = ::read(s, &f, sizeof(f));
        if (n != (ssize_t)sizeof(f)) continue;

        if (f.can_id == ID_REQ_BLE) {
            std::cout << "[router] ID_REQ_BLE received (dlc=" << (int)f.can_dlc << ")\n";

            // CAN ���̷ε� 8����Ʈ�� �ؽ÷� ���(�ɼ�)
            std::string hash16 = DEFAULT_HASH;
            if (f.can_dlc == 8) {
                static const char* HEX = "0123456789ABCDEF";
                std::string h; h.reserve(16);
                for (int i = 0; i < 8; i++) { h.push_back(HEX[(f.data[i] >> 4) & 0xF]); h.push_back(HEX[f.data[i] & 0xF]); }
                hash16 = h;
                std::cout << "[router] use hash from CAN: " << hash16 << "\n";
            }

            std::string written;
            int rc = run_ble_and_capture(hash16, ADV_NAME, BLE_TIMEOUT, written);
            can_send_byte(s, ID_RSP_BLE, (rc == 0) ? 0x00 : 0xFF);
        }
        else if (f.can_id == ID_REQ_NFC) {
            std::cout << "[router] ID_REQ_NFC received\n";
            bool match = false; std::string uid;
            int rc = run_nfc_and_judge(match, uid);
            can_send_byte(s, ID_RSP_NFC, match ? 0x00 : 0xFF);
        }
    }
    return 0;
}
