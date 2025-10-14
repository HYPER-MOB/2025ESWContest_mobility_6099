#include "can_iface.hpp"
void CANInterface::close() { if (s_ >= 0) { ::close(s_); s_ = -1; } }


bool CANInterface::send(uint32_t id, const uint8_t* data, uint8_t dlc) {
	if (s_ < 0 || dlc > 8) return false;
	can_frame f{}; f.can_id = id; f.can_dlc = dlc; std::memcpy(f.data, data, dlc);
	return ::write(s_, &f, sizeof(f)) == (ssize_t)sizeof(f);
}


std::optional<struct can_frame> CANInterface::recv(int timeout_ms) {
	if (s_ < 0) return std::nullopt;
	if (timeout_ms >= 0) {
		fd_set rf; FD_ZERO(&rf); FD_SET(s_, &rf);
		timeval tv{ timeout_ms / 1000, (timeout_ms % 1000) * 1000 };
		int r = select(s_ + 1, &rf, nullptr, nullptr, &tv);
		if (r <= 0) return std::nullopt;
	}
	can_frame f{}; ssize_t n = ::read(s_, &f, sizeof(f));
	if (n != (ssize_t)sizeof(f)) return std::nullopt;
	return f;
}


bool CANInterface::applyFilters(const std::vector<uint32_t>& ids) {
	if (s_ < 0) return false;
	if (ids.empty()) return true; // 필터 없음 = 모든 프레임 수신
	std::vector<can_filter> flt(ids.size());
	for (size_t i = 0; i < ids.size(); ++i) { flt[i].can_id = ids[i]; flt[i].can_mask = CAN_SFF_MASK; }
	return 0 == setsockopt(s_, SOL_CAN_RAW, CAN_RAW_FILTER, flt.data(), flt.size() * sizeof(can_filter));
}