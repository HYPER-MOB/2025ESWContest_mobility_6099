#pragma once
#include <string>
#include <array>
#include "can_api.hpp"     // CanFrame, can_send, can_recv ���
#include "nfc_reader.hpp"  // NFC ����
#include "sca_ble_peripheral.hpp" // BLE ����

class Sequencer {
public:
	void start();
	void stop();

	// CAN ���źΰ� �״�� ȣ��
	void on_can_frame(const CanFrame& f);

private:
	// ���� �ڵ鷯
	void handle_ble_req(const CanFrame& f);
	void handle_nfc_req(const CanFrame& f);

	// ���� (���ϸ� tcu_store ������ ���� ��)
	std::string expected_nfc_uid_ = "04AABBCCDDEEFF";
	std::string adv_name_ = "SCA-CAR";
	int         ble_timeout_ = 60;
};
