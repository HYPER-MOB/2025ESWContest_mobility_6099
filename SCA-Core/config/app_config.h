#pragma once

// ���� CAN �������̽�
#define CAN_IFACE               "can0"

// ���� �ܺ� ���� ���̳ʸ� ��� (BLE/Camera)
#define BLE_BIN_PATH            "./sca_ble_peripheral"   // ble/ ���� ����� ���������� ��Ʈ�� ��µ�(�Ʒ� ble/CMake ����)
#define CAMERA_BIN_PATH         "./camera_app"           // �װ� ���� ī�޶� ����� �̸�/��ο� �°� �ٲٸ� ��

// BLE ���� �Ķ����
#define BLE_NAME                "SCA-CAR"
#define BLE_TIMEOUT_SEC         60
#define BLE_REQUIRE_ENCRYPT     0

// Camera �Ķ����
#define CAMERA_TIMEOUT_SEC      10     // �ʿ� �� ����
// ī�޶� �� stdout���� "USER=<id>" �� ���� �������ٰ� ���� (�Ʒ� camera_runner.cpp �Ľ� ��Ģ)

// �ֱ�/ƽ(ms)
#define AUTH_STATE_PERIOD_MS    20
#define TX_TICK_MS              5
