#pragma once
#include "common.hpp"
#include <string>


/**
* NFCModule
* - NFC 리더 초기화 및 폴링 루프를 관리.
* - 태그 감지 시 UDS(JSON) `{ "type":"nfc_ok", "uid":"...", "token":"..." }` 전송.
* - 현재는 데모 스텁(주기적 가짜 이벤트); 실제 구현은 pcsc-lite 또는 libnfc 연동 지점.
*/
class NFCModule {
public:
	bool init(); ///< 드라이버/리더 핸들 준비 (실구현 시)
	bool pollStart(); ///< 폴링 루프 시작 (태그 감지 시 이벤트 발행)
	void pollStop(); ///< 폴링 루프 중지


	NFCModule(const std::string& uds_path) : uds_path_(uds_path) {}
private:
	std::string uds_path_; ///< 이벤트 전송 대상 UDS 경로
};