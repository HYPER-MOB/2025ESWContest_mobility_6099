#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <functional>


// ����Ʈ �迭 ��Ī
using bytes = std::vector<uint8_t>;


// �ð� Ÿ�� (���� ID �� ��꿡 ���)
using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;


// BLE ���� �Ľ� ��� (�ʿ� �� Ȯ��)
struct BleAdv {
	std::string mac; ///< "AA:BB:CC:DD:EE:FF"
	int8_t rssi = 0; ///< ���� RSSI(dBm)
	bytes raw; ///< AD(Advertising Data) ����
	std::optional<std::string> name; ///< ���� �̸�(�ִٸ�)
	std::optional<bytes> service_uuid; ///< ù 128-bit ���� UUID(AD �� LE �����)
};


// NFC �±� ���� (�ڸ�ǥ����)
struct NfcTag {
	bytes uid; ///< �±� UID
	std::optional<bytes> token; ///< (�ɼ�) NDEF/���Ϳ��� ���� ��ū
};


// �� ���ڿ� ��ƿ (����� �α׿�)
inline std::string hex(const bytes& v) {
	static const char* k = "0123456789ABCDEF";
	std::string s; s.reserve(v.size() * 2);
	for (auto b : v) { s.push_back(k[b >> 4]); s.push_back(k[b & 0xF]); }
	return s;
}