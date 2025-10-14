#pragma once
#include <linux/can.h>
#include <linux/can/raw.h>
#include <string>
#include <optional>
#include <vector>
#include <cstdint>
#include "can_db.hpp"


/**
* CANInterface
* - Linux SocketCAN으로 **Private CAN** 채널을 제어.
* - 수신 **하드 필터** 적용(SOL_CAN_RAW/CAN_RAW_FILTER)로 안전/성능 확보.
* - `send(const CanFrame&)` 오버로드 제공 → can_db와 자연스럽게 연동.
*/
class CANInterface {
public:
	bool open(const std::string& ifname = "can0"); ///< CAN 인터페이스 오픈(예: can0=Private)
	void close(); ///< 소켓 닫기


	bool send(uint32_t id, const uint8_t* data, uint8_t dlc); ///< 로우 송신
	bool send(const CanFrame& f) { return send(f.id, f.data, f.dlc); } ///< can_db 오버로드


	std::optional<struct can_frame> recv(int timeout_ms = -1); ///< 선택적 타임아웃 수신


	bool applyFilters(const std::vector<uint32_t>& ids); ///< ID 화이트리스트 필터 적용


private:
	int s_ = -1; ///< CAN 소켓 fd
};