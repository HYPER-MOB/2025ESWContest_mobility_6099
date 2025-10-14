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
* - Linux SocketCAN���� **Private CAN** ä���� ����.
* - ���� **�ϵ� ����** ����(SOL_CAN_RAW/CAN_RAW_FILTER)�� ����/���� Ȯ��.
* - `send(const CanFrame&)` �����ε� ���� �� can_db�� �ڿ������� ����.
*/
class CANInterface {
public:
	bool open(const std::string& ifname = "can0"); ///< CAN �������̽� ����(��: can0=Private)
	void close(); ///< ���� �ݱ�


	bool send(uint32_t id, const uint8_t* data, uint8_t dlc); ///< �ο� �۽�
	bool send(const CanFrame& f) { return send(f.id, f.data, f.dlc); } ///< can_db �����ε�


	std::optional<struct can_frame> recv(int timeout_ms = -1); ///< ������ Ÿ�Ӿƿ� ����


	bool applyFilters(const std::vector<uint32_t>& ids); ///< ID ȭ��Ʈ����Ʈ ���� ����


private:
	int s_ = -1; ///< CAN ���� fd
};