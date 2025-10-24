#pragma once
#include <cstdint>

namespace CANID {
	constexpr uint32_t DCU_SCA_USER_FACE_REQ = 0x101; // 시퀀스 트리거
	constexpr uint32_t SCA_DCU_AUTH_STATE = 0x103; // [step, state] 20ms
	constexpr uint32_t TCU_SCA_USER_INFO_NFC = 0x107; // 기대 NFC UID(8B)
	constexpr uint32_t TCU_SCA_USER_INFO_BLE_SESS = 0x108; // BLE 해시 앞 8B
	constexpr uint32_t SCA_TCU_USER_INFO_ACK = 0x111; // [index, state]
	constexpr uint32_t SCA_DCU_AUTH_RESULT = 0x112; // [flag, user_id(7B)]
}

enum class AuthStep : uint8_t { Idle = 0, NFC = 1, BLE = 2, Camera = 3, Done = 4, Error = 255 };
enum class AuthFlag : uint8_t { OK = 0, WARN = 1, FAIL = 2 };
