#pragma once

// ── CAN 인터페이스
#define CAN_IFACE               "can0"

// ── 외부 실행 바이너리 경로 (BLE/Camera)
#define BLE_BIN_PATH            "./sca_ble_peripheral"   // ble/ 에서 빌드된 실행파일이 루트로 출력됨(아래 ble/CMake 참고)
#define CAMERA_BIN_PATH         "./camera_app"           // 네가 가진 카메라 실행기 이름/경로에 맞게 바꾸면 됨

// BLE 실행 파라미터
#define BLE_NAME                "SCA-CAR"
#define BLE_TIMEOUT_SEC         60
#define BLE_REQUIRE_ENCRYPT     0

// Camera 파라미터
#define CAMERA_TIMEOUT_SEC      10     // 필요 시 조정
// 카메라 앱 stdout에서 "USER=<id>" 한 줄을 내보낸다고 가정 (아래 camera_runner.cpp 파싱 규칙)

// 주기/틱(ms)
#define AUTH_STATE_PERIOD_MS    20
#define TX_TICK_MS              5
