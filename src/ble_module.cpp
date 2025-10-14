#include "ble_module.hpp"
le_set_scan_parameters_cp scan_params{};
scan_params.type = 0x00; // passive
scan_params.interval = htobs(0x0010);
scan_params.window = htobs(0x0010);
scan_params.own_bdaddr_type = 0x00; // public
scan_params.filter = 0x00; // accept all
if (hci_send_cmd(dev_, OGF_LE_CTL, OCF_LE_SET_SCAN_PARAMETERS,
	LE_SET_SCAN_PARAMETERS_CP_SIZE, &scan_params) < 0) {
	return false;
}
// 스캔 활성화
le_set_scan_enable_cp enable_cp{};
enable_cp.enable = 0x01; // enable
enable_cp.filter_dup = 0x01;
if (hci_send_cmd(dev_, OGF_LE_CTL, OCF_LE_SET_SCAN_ENABLE,
	LE_SET_SCAN_ENABLE_CP_SIZE, &enable_cp) < 0) {
	return false;
}
running_ = true;
th_ = std::thread(&BLEModule::runLoop_, this);
return true;
}


void BLEModule::scanStop() {
	if (!running_) return;
	running_ = false;
	if (th_.joinable()) th_.join();
	le_set_scan_enable_cp disable{}; disable.enable = 0x00; disable.filter_dup = 0x00;
	hci_send_cmd(dev_, OGF_LE_CTL, OCF_LE_SET_SCAN_ENABLE, LE_SET_SCAN_ENABLE_CP_SIZE, &disable);
	if (dev_ >= 0) { close(dev_); dev_ = -1; }
}


void BLEModule::runLoop_() {
	// HCI 이벤트 필터: LE META(Advertising Report)
	struct hci_filter nf; hci_filter_clear(&nf);
	hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
	hci_filter_set_event(EVT_LE_META_EVENT, &nf);
	setsockopt(dev_, SOL_HCI, HCI_FILTER, &nf, sizeof(nf));


	EventBus bus; // 클라이언트 전송에만 사용
	while (running_) {
		uint8_t buf[HCI_MAX_EVENT_SIZE];
		int n = read(dev_, buf, sizeof(buf));
		if (n <= 0) continue;


		evt_le_meta_event* me = (evt_le_meta_event*)(buf + (1 + HCI_EVENT_HDR_SIZE));
		if (me->subevent != EVT_LE_ADVERTISING_REPORT) continue;


		auto* reports = (le_advertising_info*)(me->data + 1);
		for (int i = 0; i < me->data[0]; ++i) {
			BleAdv adv{};
			char addr[18]; ba2str(&reports->bdaddr, addr); adv.mac = addr;
			adv.rssi = (int8_t)reports->data[reports->length];
			adv.raw.assign(reports->data, reports->data + reports->length);
			adv.name = parse_name(reports->data, reports->length);
			adv.service_uuid = first_uuid128(reports->data, reports->length);


			bool pass = true;
			if (uuid_prefix_le_) pass = matchUuidPrefix_(adv, *uuid_prefix_le_);
			if (pass) {
				json j{
				{"type","ble_ok"},
				{"mac", adv.mac},
				{"rssi", adv.rssi},
				{"name", adv.name.value_or("")},
				{"svc_uuid_le", adv.service_uuid ? hex(*adv.service_uuid) : ""}
				};
				bus.send(uds_path_, j); // Orchestrator(UDS 서버)로 이벤트 전송
			}


			reports = (le_advertising_info*)((uint8_t*)reports + reports->length + 2 + 6 + 1);
		}
	}
}


bool BLEModule::matchUuidPrefix_(const BleAdv& adv, const bytes& prefix_le) {
	if (!adv.service_uuid) return false;
	const bytes& u = *adv.service_uuid;
	if (prefix_le.size() > u.size()) return false;
	for (size_t i = 0; i < prefix_le.size(); ++i) if (u[i] != prefix_le[i]) return false;
	return true;
}