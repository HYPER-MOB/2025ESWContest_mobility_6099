#pragma once
#include <string>

// BLE ���� �ɼ�(�ʿ� �� Ȯ��)
struct BleOptions {
    std::string hash_last12;   // UUID�� ������ 12-hex (��: "A1B2C3D4E5F6")
    std::string local_name = "SCA-CAR";
    int         timeout_sec = 30;
    bool        require_encrypt = false;
    std::string expected_token = "ACCESS"; // Ư�� Write payload ��밪
};

// 12-hex�� ���� �� �� ��� (���� 0, ����/Ÿ�Ӿƿ� 1)
int run_ble_peripheral_12hash(const BleOptions& opt);

// 16-hex(8����Ʈ)�� ������, ���ο��� 12�ڷ� �߶� ��� (���� 0, ���� 1)
// ��Ģ: �� 12�ڸ� ���. (�ٸ��� ���ϸ� ���⼭ �����̽� ��å�� �ٲٸ� ��)
int run_ble_peripheral_16hash(const std::string& hash16,
    const std::string& local_name,
    int timeout_sec,
    bool require_encrypt,
    const std::string& expected_token);
