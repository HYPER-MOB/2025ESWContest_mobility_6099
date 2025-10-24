#pragma once
#include <string>
#include <array>
#include "can_api.hpp"     // CanFrame, can_send, can_recv 사용
#include "nfc_reader.hpp"  // NFC 내장
#include "sca_ble_peripheral.hpp" // BLE 내장

class Sequencer {
public:
	void start();
	void stop();

	// CAN 수신부가 그대로 호출
	void on_can_frame(const CanFrame& f);

private:
	// 내부 핸들러
	void handle_ble_req(const CanFrame& f);
	void handle_nfc_req(const CanFrame& f);

	// 설정 (원하면 tcu_store 등으로 빼도 됨)
	std::string expected_nfc_uid_ = "04AABBCCDDEEFF";
	std::string adv_name_ = "SCA-CAR";
	int         ble_timeout_ = 60;
};
