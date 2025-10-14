#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <functional>


// 바이트 배열 별칭
using bytes = std::vector<uint8_t>;


// 시간 타입 (세션 ID 등 계산에 사용)
using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;


// BLE 광고 파싱 결과 (필요 시 확장)
struct BleAdv {
	std::string mac; ///< "AA:BB:CC:DD:EE:FF"
	int8_t rssi = 0; ///< 수신 RSSI(dBm)
	bytes raw; ///< AD(Advertising Data) 원본
	std::optional<std::string> name; ///< 로컬 이름(있다면)
	std::optional<bytes> service_uuid; ///< 첫 128-bit 서비스 UUID(AD 내 LE 엔디언)
};


// NFC 태그 정보 (자리표시자)
struct NfcTag {
	bytes uid; ///< 태그 UID
	std::optional<bytes> token; ///< (옵션) NDEF/섹터에서 읽은 토큰
};


// 헥스 문자열 유틸 (디버그 로그용)
inline std::string hex(const bytes& v) {
	static const char* k = "0123456789ABCDEF";
	std::string s; s.reserve(v.size() * 2);
	for (auto b : v) { s.push_back(k[b >> 4]); s.push_back(k[b & 0xF]); }
	return s;
}