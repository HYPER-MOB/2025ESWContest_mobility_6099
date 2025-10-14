#include "ble_module.hpp"
#include "nfc_module.hpp"
#include "orchestrator.hpp"
#include "can_iface.hpp"
#include "log_sink.hpp"
#include "can_db.hpp"
#include <thread>
#include <nlohmann/json.hpp>


int main() {
	// 0) 로그 파일 오픈 (운영 시 권한/로테이션/암호화 고려)
	LogSink log; log.open("/var/log/sca_core.log");


	// 1) Private CAN 오픈 (예: can0)
	CANInterface can; if (!can.open("can0")) { log.write("CAN open failed"); return 1; }


	// 2) Private CAN 하드 필터 적용 — 네 표의 ID만 수신
	std::vector<uint32_t> ids = {
	CANMSG::DCU_RESET, CANMSG::DCU_RESET_ACK,
	CANMSG::DCU_SCA_USER_FACE_REQ, CANMSG::SCA_TCU_USER_INFO_REQ, CANMSG::SCA_DCU_AUTH_STATE,
	CANMSG::TCU_SCA_USER_INFO, CANMSG::TCU_SCA_USER_INFO_NFC,
	CANMSG::TCU_SCA_USER_INFO_BLE_SESS, CANMSG::TCU_SCA_USER_INFO_BLE_CHAL, CANMSG::TCU_SCA_USER_INFO_BLE_FLAG,
	CANMSG::SCA_TCU_USER_INFO_ACK, CANMSG::SCA_DCU_AUTH_RESULT, CANMSG::SCA_DCU_AUTH_RESULT_ADD,
	CANMSG::DCU_TCU_USER_PROFILE_REQ, CANMSG::TCU_DCU_USER_PROFILE_SEAT, CANMSG::TCU_DCU_USER_PROFILE_MIR,
	CANMSG::TCU_DCU_USER_PROFILE_WHL, CANMSG::DCU_TCU_USER_PROFILE_ACK,
	CANMSG::DCU_TCU_USER_PROFILE_SEAT_UPDATE, CANMSG::DCU_TCU_USER_PROFILE_MIR_UPDATE,
	CANMSG::DCU_TCU_USER_PROFILE_WHL_UPDATE, CANMSG::TCU_DCU_USER_PROFILE_UPDATE_ACK
	};
	can.applyFilters(ids);


	// 3) Orchestrator(UDS 서버) 시작
	Orchestrator orch; if (!orch.init("/tmp/sca_uds", &can, &log)) { log.write("UDS listen failed"); return 1; }


	// 4) BLE/NFC 모듈 기동
	BLEModule ble("/tmp/sca_uds");
	if (!ble.init(0)) { log.write("BLE init failed"); }
	// 필요 시: 128-bit 서비스 UUID(문자열)→LE 프리픽스 변환 후 scanStart(prefix)로 전달
	ble.scanStart(std::nullopt); // 필터 없이 시작(데모)


	NFCModule nfc("/tmp/sca_uds");
	nfc.init(); nfc.pollStart(); // 데모 스텁: 10초마다 가짜 이벤트


	// 5) Orchestrator 루프 (UDS 이벤트 처리 + 0x103 20ms 주기 송신)
	orch.run();
	return 0;
}