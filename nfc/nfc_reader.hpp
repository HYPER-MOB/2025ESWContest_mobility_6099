#pragma once
#include <string>

namespace sca {

	struct NfcConfig {
		int poll_seconds{ 5 };
	};

	struct NfcResult {
		bool ok{ false };
		bool use_apdu{ false };
		std::string uid_hex;
	};

	bool nfc_poll_once(const NfcConfig& cfg, NfcResult& out);

} // namespace sca
