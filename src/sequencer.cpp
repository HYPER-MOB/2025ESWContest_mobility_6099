#include "sequencer.hpp"
#include "config/app_config.h"
#include "tcu_store.hpp"
#include "nfc/nfc_reader.hpp"
#include "camera/camera_runner.hpp"

#include <array>
#include <cctype>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

// ── 간이 실행 캡처 (BLE 실행용)
static int run_and_capture(const std::string& cmd, std::vector<std::string>& lines) {
    std::array<char, 1024> buf{};
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return 1;
    while (fgets(buf.data(), buf.size(), p)) lines.emplace_back(buf.data());
    int st = pclose(p);
    return WIFEXITED(st) ? WEXITSTATUS(st) : 1;
}

// NFC 단계: 기대 UID(0x107) vs 리더 UID(libnfc) 비교
bool run_nfc(SeqContext& ctx, const TcuProvided& tcu) {
    ctx.step = AuthStep::NFC;
    const std::string expected_uid = TcuProvided::to_hex_uid(tcu.expected_nfc);

    std::string read_uid;
    bool got = read_nfc_uid(read_uid, /*timeout*/ BLE_TIMEOUT_SEC);
    ctx.nfc_uid_read = read_uid;

    bool ok = (got && !read_uid.empty() && read_uid == expected_uid);
    ctx.flag = ok ? AuthFlag::OK : AuthFlag::FAIL;
    return ok;
}

// BLE 단계: 외부 실행파일(네 최종 코드) 호출
bool run_ble(SeqContext& ctx, const TcuProvided& tcu) {
    ctx.step = AuthStep::BLE;
    const std::string hash16 = TcuProvided::to_hex16(tcu.ble_hash);

#ifdef USE_BLE_LIB
    // 라이브러리 직접 호출
    int rc = run_ble_peripheral_16hash(hash16, BLE_NAME, BLE_TIMEOUT_SEC, BLE_REQUIRE_ENCRYPT, "ACCESS");
#else
    // 외부 실행파일 (기존 방식)
    auto sh = [&](const std::string& cmd) { /* run_and_capture 구현 (생략) */ return 0; };
    std::ostringstream cmd;
    cmd << "timeout " << (BLE_TIMEOUT_SEC + 5) << "s "
        << BLE_BIN_PATH
        << " --hash " << hash16
        << " --name \"" << BLE_NAME << "\""
        << " --timeout " << BLE_TIMEOUT_SEC
        << " --require-encrypt " << BLE_REQUIRE_ENCRYPT;
    int rc = sh(cmd.str());
#endif

    ctx.ble_ok = (rc == 0);
    ctx.flag = ctx.ble_ok ? AuthFlag::OK : AuthFlag::FAIL;
    return ctx.ble_ok;
}

// 카메라 단계: 외부 앱 실행 후 USER=... 파싱
bool run_camera(SeqContext& ctx) {
    ctx.step = AuthStep::Camera;
    std::string uid;
    bool ok = run_camera_app(uid, CAMERA_TIMEOUT_SEC);
    if (ok && uid.empty()) ok = false;
    ctx.user_id = ok ? uid : "";
    ctx.flag = ok ? AuthFlag::OK : AuthFlag::FAIL;
    ctx.step = ok ? AuthStep::Done : AuthStep::Error;
    return ok;
}
