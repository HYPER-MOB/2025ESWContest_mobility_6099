#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <nlohmann/json.hpp>
#include "can_iface.hpp"
#include "log_sink.hpp"


/**
* Orchestrator
* - **UDS 서버**를 열고(BLE/NFC 등에서 올라오는 이벤트 수신), 설계 정책에 따라 **CAN 송신**을 조율.
* - **세션 상태(active/fid)**를 관리하고, 활성 중일 때 **0x103(SCA_DCU_AUTH_STATE)**를 **20ms 주기**로 송신.
* - (옵션) CAN 수신 스레드로 TCU/DCU 메시지를 관찰 → 필요 시 내부 이벤트로 변환.
*/
class Orchestrator {
public:
	bool init(const std::string& uds_path, CANInterface* can, LogSink* log);
	void run(); ///< UDS 수신 루프(블로킹)
private:
	void onEvent(const nlohmann::json& j); ///< JSON 이벤트 라우팅
	void handleBleOk_(const nlohmann::json& j);
	void handleNfcOk_(const nlohmann::json& j);


	void startAuthStateTx_(); ///< 20ms 주기 송신 스레드 시작
	void stopAuthStateTx_(); ///< 주기 송신 스레드 종료
	void canRecvLoop_(); ///< (옵션) CAN 수신 → 로그/처리


	// 상태
	std::string uds_path_;
	CANInterface* can_ = nullptr;
	LogSink* log_ = nullptr;


	std::atomic<bool> active_{ false }; ///< 활성 세션 여부
	uint32_t fid_ = 0; ///< 세션/플로우 ID
	std::thread tx_thread_;
	std::atomic<bool> tx_run_{ false };
	std::thread can_rx_thread_;
	std::atomic<bool> can_rx_run_{ false };


	// 예시: 인증 단계/상태 (실제 정책에 맞게 업데이트)
	std::atomic<uint8_t> auth_step_{ 0 };
	std::atomic<uint8_t> auth_state_{ 0 };
};