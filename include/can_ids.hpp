#pragma once
#include <cstdint>

// ===== 프로젝트 공통 CAN ID (11-bit) =====
enum : uint32_t {
    // DCU ↔ SCA
    BCAN_ID_DCU_RESET = 0x001,
    BCAN_ID_DCU_RESET_ACK = 0x002,

    BCAN_ID_DCU_SCA_USER_FACE_REQ = 0x101, // DCU -> SCA : 인증 시작 트리거
    BCAN_ID_SCA_TCU_USER_INFO_REQ = 0x102, // SCA -> TCU : 사용자 인증정보 요청
    BCAN_ID_SCA_DCU_AUTH_STATE = 0x103, // SCA -> DCU : 단계/상태 알림(20ms 주기 권장이나 지금은 이벤트성)
    BCAN_ID_TCU_SCA_USER_INFO = 0x104, // (미사용: Facemesh 용)

    BCAN_ID_TCU_SCA_USER_INFO_NFC = 0x107, // TCU -> SCA : NFC 기대값(8바이트)
    BCAN_ID_TCU_SCA_USER_INFO_BLE_SESS = 0x108, // TCU -> SCA : BLE 세션(32비트 등.. 임시 사용)
    BCAN_ID_TCU_SCA_USER_INFO_BLE_CHAL = 0x109, // TCU -> SCA : BLE challenge(32비트 등.. 임시 사용)
    BCAN_ID_TCU_SCA_USER_INFO_BLE_FLAG = 0x110, // (미정)

    BCAN_ID_SCA_TCU_USER_INFO_ACK = 0x111, // SCA -> TCU : 수신 ACK (index/state)

    BCAN_ID_SCA_DCU_AUTH_RESULT = 0x112, // SCA -> DCU : 인증 결과 + user_id(7B)
    BCAN_ID_SCA_DCU_AUTH_RESULT_ADD = 0x113, // SCA -> DCU : user_id 추가(필요시)

    // 프로파일 (지금은 X)
    BCAN_ID_DCU_TCU_USER_PROFILE_REQ = 0x201,
    BCAN_ID_TCU_DCU_USER_PROFILE_SEAT = 0x202,
    BCAN_ID_TCU_DCU_USER_PROFILE_MIRROR = 0x203,
    BCAN_ID_TCU_DCU_USER_PROFILE_WHEEL = 0x204,
    BCAN_ID_DCU_TCU_USER_PROFILE_ACK = 0x205,
    BCAN_ID_DCU_TCU_USER_PROFILE_SEAT_UPDATE = 0x206,
    BCAN_ID_DCU_TCU_USER_PROFILE_MIRROR_UPDATE = 0x207,
    BCAN_ID_DCU_TCU_USER_PROFILE_WHEEL_UPDATE = 0x208,
    BCAN_ID_TCU_DCU_USER_PROFILE_UPDATE_ACK = 0x209,
};

// 간단 signal packing helper
static inline uint8_t clamp8(int v) { return (v < 0) ? 0 : ((v > 255) ? 255 : v); }
