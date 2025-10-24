// CAN Router (C++): CAN → (외부 실행: BLE/NFC) → CAN 응답
// - ID_REQ_BLE(0x100) 수신 → ./sca_ble_peripheral --hash <HEX> --name SCA-CAR --timeout 60
//   종료코드 0 → ID_RSP_BLE(0x110, 0x00), 아니면 0xFF
// - ID_REQ_NFC(0x101) 수신 → ./test_nfc_log, UID 파싱 → 기대값 매치면 ID_RSP_NFC(0x111, 0x00), 아니면 0xFF
//
// 실행파일(BLE/NFC)은 "build/"에서 router_canapi와 같은 폴더에 있다고 가정.
//   build/
//     ├─ router_canapi
//     ├─ sca_ble_peripheral
//     └─ test_nfc_log

#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>
#include <filesystem>
#include <cctype>

#include "can_api.hpp"
#include "canmessage.hpp"

// ---- CAN IDs ----
static const uint32_t ID_REQ_BLE = 0x100;
static const uint32_t ID_REQ_NFC = 0x101;
static const uint32_t ID_RSP_BLE = 0x110;
static const uint32_t ID_RSP_NFC = 0x111;

// ---- Parameters ----
static const char* DEFAULT_HASH = "A1B2C3D4E5F6";
static const char* ADV_NAME = "SCA-CAR";
static const int   BLE_TIMEOUT = 60;

// NFC 기대 UID(대문자/공백없음)
static const char* EXPECTED_NFC_UID = "04AABBCCDDEEFF";

static const char* CH = "can0";
static std::atomic_bool     g_stop{ false };

// --- 경로 유틸: 실행 파일(라우터) 디렉토리 ---
static std::filesystem::path get_self_dir(const char* argv0) {
    std::error_code ec;
    auto exe = std::filesystem::weakly_canonical(std::filesystem::path(argv0), ec);
    if (ec) exe = std::filesystem::path(argv0); // 플랜B
    auto dir = exe.parent_path();
    if (dir.empty()) dir = std::filesystem::current_path();
    return dir;
}

static void on_sigint(int) { g_stop.store(true); }

static void send_rsp_byte(uint32_t canid, uint8_t code) {
    CanFrame fr{}; fr.id = canid; fr.dlc = 1; fr.data[0] = code;
    can_err_t r = can_send(CH, fr, 0);
    if (r != CAN_OK) {
        std::fprintf(stderr, "[router] send_rsp_byte(0x%X) error=%d\n", (unsigned)canid, r);
    }
}

static std::string bytes_to_hex(const uint8_t* d, size_t n) {
    static const char* H = "0123456789ABCD";
    std::string s; s.resize(n * 2);
    for (size_t i = 0; i < n; i++) {
        s[i * 2 + 0] = H[(d[i] >> 4) & 0xF];
        s[i * 2 + 1] = H[(d[i]) & 0xF];
    }
    return s;
}

// 공통: 외부 실행 + stdout 캡처 + 로그 파일도 기록
static int run_and_capture(const std::string& cmd, const std::string& logfile, std::vector<std::string>& out) {
    std::filesystem::create_directories(std::filesystem::path(logfile).parent_path());
    FILE* fp = popen(cmd.c_str(), "r");
    FILE* lg = std::fopen(logfile.c_str(), "a");
    if (!fp) {
        if (lg) { std::fprintf(lg, "[router] popen failed: %s\n", cmd.c_str()); std::fclose(lg); }
        std::fprintf(stderr, "[router] popen failed: %s\n", cmd.c_str());
        return 1;
    }
    std::fprintf(stdout, "[router] spawn: %s\n", cmd.c_str());
    if (lg) std::fprintf(lg, "[router] spawn: %s\n", cmd.c_str());

    char buf[1024];
    while (std::fgets(buf, sizeof(buf), fp)) {
        if (lg) std::fputs(buf, lg);
        std::fputs(buf, stdout);
        out.emplace_back(buf);
    }
    int status = pclose(fp);
    int exitcode = (WIFEXITED(status) ? WEXITSTATUS(status) : 1);
    if (lg) { std::fprintf(lg, "[router] proc exit=%d\n", exitcode); std::fclose(lg); }
    return exitcode;
}

static void parse_ble_written(const std::vector<std::string>& lines, std::string& out) {
    out.clear();
    for (auto& s : lines) {
        auto pos = s.find("data=");
        if (pos != std::string::npos) {
            out = s.substr(pos + 5);
            while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
        }
    }
}

static bool parse_uid_line(const std::string& s, std::string& uid) {
    auto p = s.find("UID");
    if (p == std::string::npos) return false;
    auto q = s.find_first_of("=:", p);
    if (q == std::string::npos) return false;
    q++;
    std::string hex;
    hex.reserve(64);
    for (; q < s.size(); ++q) {
        unsigned char c = (unsigned char)s[q];
        if (std::isxdigit(c)) hex.push_back((char)std::toupper(c));
    }
    if (hex.size() >= 6) { uid = std::move(hex); return true; }
    return false;
}

// ---- BLE / NFC 워커 ----
static void ble_worker(CanFrame req, const std::filesystem::path baseDir) {
    std::string hash16 = (req.dlc == 8) ? bytes_to_hex(req.data, 8) : DEFAULT_HASH;
    std::printf("[router] use hash: %s\n", hash16.c_str());

    auto exe = (baseDir / "sca_ble_peripheral").string();
    char cmd[768];
    std::snprintf(cmd, sizeof(cmd),
        "%s --hash %s --name %s --timeout %d",
        exe.c_str(), hash16.c_str(), ADV_NAME, BLE_TIMEOUT);

    std::vector<std::string> lines;
    int rc = run_and_capture(cmd, (baseDir / "ble_run.log").string(), lines);

    std::string written; parse_ble_written(lines, written);
    std::printf("[router] BLE captured=\"%s\" exit=%d\n", written.c_str(), rc);

    send_rsp_byte(ID_RSP_BLE, (rc == 0) ? 0x00 : 0xFF);
}

static void nfc_worker(const std::filesystem::path baseDir) {
    auto exe = (baseDir / "test_nfc_log").string();
    std::vector<std::string> lines;
    int rc = run_and_capture(exe, (baseDir / "nfc_run.log").string(), lines);

    std::string uid;
    for (auto& s : lines) {
        if (parse_uid_line(s, uid)) break;
    }
    bool match = (!uid.empty() && uid == EXPECTED_NFC_UID);
    std::printf("[router] NFC uid=\"%s\" expected=\"%s\" match=%s exit=%d\n",
        uid.c_str(), EXPECTED_NFC_UID, match ? "true" : "false", rc);

    send_rsp_byte(ID_RSP_NFC, match ? 0x00 : 0xFF);
}

// ---- CAN 수신 콜백 ----
static void on_rx(const CanFrame* f, void* user) {
    const auto* baseDir = static_cast<const std::filesystem::path*>(user);

    std::printf("[RX] id=0x%X dlc=%u :", (unsigned)f->id, f->dlc);
    for (uint8_t i = 0; i < f->dlc && i < 8; i++) std::printf(" %02X", f->data[i]);
    std::puts("");

    if (f->id == ID_REQ_BLE) {
        std::thread(ble_worker, *f, *baseDir).detach();
    }
    else if (f->id == ID_REQ_NFC) {
        std::thread(nfc_worker, *baseDir).detach();
    }
}

int main(int argc, char** argv) {
    std::signal(SIGINT, on_sigint);
    std::signal(SIGTERM, on_sigint);

    const auto baseDir = get_self_dir(argv[0]);

    if (can_init(CAN_DEVICE_LINUX) != CAN_OK) {   // 실버스면 CAN_DEVICE_LINUX
        std::fprintf(stderr, "can_init failed\n");
        return 1;
    }

    CanConfig cfg{};
    cfg.channel     = 0;
    cfg.bitrate     = 500000;
    cfg.samplePoint = 0.875f;
    cfg.sjw         = 1;
    cfg.mode        = CAN_MODE_NORMAL;

    if (can_open(CH, cfg) != CAN_OK) {
        std::fprintf(stderr, "can_open failed\n");
        return 1;
    }

    // 전체 구독 후 ID 분기(필요하면 마스크 좁혀도 됨)    
    CanFilter any{};
    any.type = CAN_FILTER_MASK;
    any.data.mask.id = 0x000;
    any.data.mask.mask = 0x000;
    if (can_subscribe(CH, any, on_rx, (void*)&baseDir) <= 0) {
        std::fprintf(stderr, "can_subscribe failed\n");
        return 1;
    }

    std::puts("[router_canapi] ready. Waiting CAN requests (BLE/NFC) …");
    while (!g_stop.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    can_dispose();
    return 0;
}
