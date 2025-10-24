#pragma once
#include <string>

namespace sca {

	struct NfcConfig {
		int poll_seconds{ 5 };    // 폴링 시간 제한
	};

	struct NfcResult {
		bool ok{ false };         // UID를 읽었는지
		std::string uid_hex;    // 읽힌 UID (대문자 HEX 연속문자)
	};

	// 간단 libnfc 기반 폴링(내장). 성공 시 true
	bool nfc_poll_once(const NfcConfig& cfg, NfcResult& out);

} // namespace sca
