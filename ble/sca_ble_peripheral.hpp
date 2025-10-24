#pragma once
#include <string>

// BLE 실행 옵션(필요 시 확장)
struct BleOptions {
    std::string hash_last12;   // UUID의 마지막 12-hex (예: "A1B2C3D4E5F6")
    std::string local_name = "SCA-CAR";
    int         timeout_sec = 30;
    bool        require_encrypt = false;
    std::string expected_token = "ACCESS"; // 특성 Write payload 기대값
};

// 12-hex를 직접 줄 때 사용 (성공 0, 실패/타임아웃 1)
int run_ble_peripheral_12hash(const BleOptions& opt);

// 16-hex(8바이트)로 들어오면, 내부에서 12자로 잘라서 사용 (성공 0, 실패 1)
// 규칙: 앞 12자를 사용. (다르게 원하면 여기서 슬라이스 정책만 바꾸면 됨)
int run_ble_peripheral_16hash(const std::string& hash16,
    const std::string& local_name,
    int timeout_sec,
    bool require_encrypt,
    const std::string& expected_token);
