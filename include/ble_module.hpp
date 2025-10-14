#pragma once
#include "common.hpp"
#include <atomic>
#include <thread>
#include <nlohmann/json.hpp>


/**
* BLEModule
* - BlueZ **HCI raw socket**으로 LE 광고(EVT_LE_ADVERTISING_REPORT)를 직접 수신.
* - **D-Bus 의존성 없음**: 데몬(bluetoothd) 없이도 단일 바이너리로 동작.
* - AD 내부 **128-bit 서비스 UUID(LE 엔디언)** 프리픽스가 주어지면 프리필터링.
* - 통과한 광고를 **UDS(JSON)** 이벤트 `{ "type":"ble_ok", ... }`로 Orchestrator에 발행.
*/
class BLEModule {
public:
	/** @brief HCI 디바이스 오픈 (기본 hci0) */
	bool init(int hci_index = 0);


	/**
	* @brief 스캔 시작
	* @param service_uuid_prefix 128-bit 서비스 UUID의 **LE 바이트열 프리픽스**(선택)
	* @return 성공 여부
	*/
	bool scanStart(const std::optional<bytes>& service_uuid_prefix = std::nullopt);


	/** @brief 스캔 중지(스레드 join) 및 디바이스 정리 */
	void scanStop();


	/** @brief 이벤트를 보낼 UDS 경로를 지정하는 생성자 */
	BLEModule(const std::string& uds_path) : uds_path_(uds_path) {}
	~BLEModule() { scanStop(); }


private:
	void runLoop_(); ///< 수신 루프 (별도 스레드)
	bool parseAdv_(const uint8_t* data, size_t len, BleAdv& out); ///< (내부 유틸) AD 파싱
	static bool matchUuidPrefix_(const BleAdv& adv, const bytes& prefix_le); ///< LE 프리픽스 매칭


	int dev_ = -1; ///< HCI raw 소켓 fd
	std::atomic<bool> running_{ false }; ///< 스캔 루프 동작 플래그
	std::thread th_; ///< 스캔 스레드
	std::string uds_path_; ///< Orchestrator UDS 경로
	std::optional<bytes> uuid_prefix_le_; ///< 128-bit UUID(LE) 프리픽스
};