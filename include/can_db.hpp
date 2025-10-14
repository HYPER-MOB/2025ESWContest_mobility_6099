#pragma once
constexpr uint32_t SCA_DCU_AUTH_RESULT = 0x112; // DLC=8 (flag + user_id[7])
constexpr uint32_t SCA_DCU_AUTH_RESULT_ADD = 0x113; // DLC=8 (user_id 이어붙임)


constexpr uint32_t DCU_TCU_USER_PROFILE_REQ = 0x201; // DLC=1
constexpr uint32_t TCU_DCU_USER_PROFILE_SEAT = 0x202; // DLC=4
constexpr uint32_t TCU_DCU_USER_PROFILE_MIR = 0x203; // DLC=6
constexpr uint32_t TCU_DCU_USER_PROFILE_WHL = 0x204; // DLC=2
constexpr uint32_t DCU_TCU_USER_PROFILE_ACK = 0x205; // DLC=2
constexpr uint32_t DCU_TCU_USER_PROFILE_SEAT_UPDATE = 0x206; // DLC=4
constexpr uint32_t DCU_TCU_USER_PROFILE_MIR_UPDATE = 0x207; // DLC=6
constexpr uint32_t DCU_TCU_USER_PROFILE_WHL_UPDATE = 0x208; // DLC=2
constexpr uint32_t TCU_DCU_USER_PROFILE_UPDATE_ACK = 0x209; // DLC=2
}


// === Pack(송신)/Unpack(수신) 헬퍼 ===
namespace PACK {
	inline CanFrame dcu_reset() {
		CanFrame f; f.id = CANMSG::DCU_RESET; f.dlc = 1; f.data[0] = 0x01; return f;
	}
	inline CanFrame dcu_reset_ack(uint8_t index, uint8_t status) {
		CanFrame f; f.id = CANMSG::DCU_RESET_ACK; f.dlc = 2; f.data[0] = index; f.data[1] = status; return f;
	}
	inline CanFrame dcu_sca_user_face_req() {
		CanFrame f; f.id = CANMSG::DCU_SCA_USER_FACE_REQ; f.dlc = 1; f.data[0] = 0x01; return f;
	}
	inline CanFrame sca_tcu_user_info_req() {
		CanFrame f; f.id = CANMSG::SCA_TCU_USER_INFO_REQ; f.dlc = 1; f.data[0] = 0x01; return f;
	}
	inline CanFrame sca_dcu_auth_state(uint8_t step, uint8_t state) {
		CanFrame f; f.id = CANMSG::SCA_DCU_AUTH_STATE; f.dlc = 2; f.data[0] = step; f.data[1] = state; return f;
	}
	inline CanFrame sca_tcu_user_info_ack(uint8_t index, uint8_t ack_state) {
		CanFrame f; f.id = CANMSG::SCA_TCU_USER_INFO_ACK; f.dlc = 2; f.data[0] = index; f.data[1] = ack_state; return f;
	}
	inline CanFrame sca_dcu_auth_result(uint8_t flag, const uint8_t user7[7]) {
		CanFrame f; f.id = CANMSG::SCA_DCU_AUTH_RESULT; f.dlc = 8; f.data[0] = flag; for (int i = 0; i < 7; i++) f.data[1 + i] = user7[i]; return f;
	}
	inline CanFrame sca_dcu_auth_result_add(const std::array<uint8_t, 8>& more) {
		CanFrame f; f.id = CANMSG::SCA_DCU_AUTH_RESULT_ADD; f.dlc = 8; for (int i = 0; i < 8; i++) f.data[i] = more[i]; return f;
	}
}


namespace UNPACK {
	inline std::optional<std::pair<uint8_t, uint8_t>> dcu_reset_ack(const CanFrame& f) {
		if (f.id != CANMSG::DCU_RESET_ACK || f.dlc != 2) return std::nullopt; return std::make_pair(f.data[0], f.data[1]);
	}
	inline std::optional<std::pair<uint8_t, uint8_t>> sca_dcu_auth_state(const CanFrame& f) {
		if (f.id != CANMSG::SCA_DCU_AUTH_STATE || f.dlc != 2) return std::nullopt; return std::make_pair(f.data[0], f.data[1]);
	}
	inline std::optional<std::array<uint8_t, 8>> tcu_sca_user_info_nfc(const CanFrame& f) {
		if (f.id != CANMSG::TCU_SCA_USER_INFO_NFC || f.dlc != 8) return std::nullopt; std::array<uint8_t, 8> r{}; for (int i = 0; i < 8; i++) r[i] = f.data[i]; return r;
	}
}