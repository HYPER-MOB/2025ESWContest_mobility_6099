#include "orchestrator.hpp"
auto j = bus.receive();
if (!j) continue;
onEvent(*j);
}
}


void Orchestrator::onEvent(const json& j) {
	const std::string type = j.value("type", "");
	if (type == "ble_ok") handleBleOk_(j);
	else if (type == "nfc_ok") handleNfcOk_(j);
	// �ʿ� ��: else if (type == "tcu_decision") {...}
}


void Orchestrator::handleBleOk_(const json& j) {
	if (!active_) { active_ = true; fid_ = (uint32_t)Clock::now().time_since_epoch().count(); startAuthStateTx_(); }
	if (log_) log_->write("BLE mac=" + j.value("mac", "") + " rssi=" + std::to_string(j.value("rssi", 0)));
	// ��å�� �°� step/state ���� ����
	auth_step_ = 1; auth_state_ = 0; // 1�ܰ� ������, �̻� ����
}


void Orchestrator::handleNfcOk_(const json& j) {
	if (!active_) { active_ = true; fid_ = (uint32_t)Clock::now().time_since_epoch().count(); startAuthStateTx_(); }
	if (log_) log_->write("NFC uid=" + j.value("uid", ""));
	// ����: NFC�� ���� ���� Ȯ�� �� �ܰ� ����/���� ������Ʈ
	auth_step_ = 2; auth_state_ = 0; // 2�ܰ� ������
}


void Orchestrator::startAuthStateTx_() {
	if (tx_run_) return;
	tx_run_ = true;
	tx_thread_ = std::thread([this] {
		while (tx_run_) {
			if (active_) {
				auto f = PACK::sca_dcu_auth_state(auth_step_.load(), auth_state_.load());
				can_->send(f); // 0x103, 20ms �ֱ� �۽�
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
		});
}


void Orchestrator::stopAuthStateTx_() {
	tx_run_ = false;
	if (tx_thread_.joinable()) tx_thread_.join();
}


void Orchestrator::canRecvLoop_() {
	// Private CAN ����: �ʿ� ID�� ���͸��Ǿ� ���´ٰ� ����
	while (can_rx_run_) {
		auto f = can_->recv(100); // 100ms Ÿ�Ӿƿ�
		if (!f) continue;
		const auto& fr = *f;
		if (log_) log_->write("CAN RX id=0x" + std::to_string(fr.can_id) + " dlc=" + std::to_string(fr.can_dlc));
		// �ʿ� ��: UNPACK�� ���� ���� ���� ����/�̺�Ʈ ����
		// ��) if (fr.can_id == CANMSG::TCU_SCA_USER_INFO_NFC) { ... }
	}
}