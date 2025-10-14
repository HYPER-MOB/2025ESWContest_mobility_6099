#pragma once
#include <string>
#include <memory>
#include <nlohmann/json.hpp>


/**
* EventBus
* - **UDS(Unix Domain Socket)** 기반 IPC. 같은 장치 내 모듈 간 초저지연 통신.
* - 메시지 프레이밍: **[4바이트 길이(LE)] + JSON 텍스트**.
* - Orchestrator가 listen()으로 **서버**를 열고, BLE/NFC 모듈은 send()로 **클라이언트 전송**.
* - 파일 퍼미션(0660)으로 접근 주체 제어(보안).
*/
class EventBus {
public:
	/**
	* @brief UDS 서버 오픈 (Orchestrator에서 호출)
	* @param path 소켓 경로 (예: "/tmp/sca_uds")
	* @param mode 파일 퍼미션 (0660 권장: 소유자/그룹 RW)
	*/
	bool listen(const std::string& path, mode_t mode = 0660);


	/**
	* @brief 블로킹 수신 — accept 1회 + 한 건 읽기
	* @return JSON 포인터 (파싱 실패/에러 시 nullptr)
	*/
	std::unique_ptr<nlohmann::json> receive();


	/**
	* @brief 클라이언트 모드로 단건 전송
	* @param path 서버 소켓 경로
	* @param j 전송할 JSON
	*/
	bool send(const std::string& path, const nlohmann::json& j);


private:
	int srv_fd_ = -1; ///< AF_UNIX 서버 소켓 fd (listen용)
};