#pragma once
#include <string>

namespace sca {

	struct NfcConfig {
		int poll_seconds{ 5 };    // ���� �ð� ����
	};

	struct NfcResult {
		bool ok{ false };         // UID�� �о�����
		std::string uid_hex;    // ���� UID (�빮�� HEX ���ӹ���)
	};

	// ���� libnfc ��� ����(����). ���� �� true
	bool nfc_poll_once(const NfcConfig& cfg, NfcResult& out);

} // namespace sca
