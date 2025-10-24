#include "sequencer.hpp"
#include <iostream>
#include <cctype>

static constexpr uint32_t ID_REQ_BLE = 0x100;
static constexpr uint32_t ID_REQ_NFC = 0x101;
static constexpr uint32_t ID_RSP_BLE = 0x110;
static constexpr uint32_t ID_RSP_NFC = 0x111;

void Sequencer::start() {
    // 필요한 초기화가 있으면 여기서
}

void Sequencer::stop() {
    // 필요한 정리가 있으면 여기서
}

void Sequencer::on_can_frame(const CanFrame& f) {
    
    if (f.id == ID_REQ_BLE) {
        handle_ble_req(f);
    }
    else if (f.id == ID_REQ_NFC) {
        handle_nfc_req(f);
    }
    else {
        // 다른 ID들은 기존 로직대로…
    }
}

static std::string bytes_to_hex16(const uint8_t* d, uint8_t dlc) {
    static const char* HEX = "0123456789ABCDEF";
    std::string s; s.reserve(16);
    for (int i = 0; i < 8 && i < dlc; i++) { s.push_back(HEX[(d[i] >> 4) & 0xF]); s.push_back(HEX[d[i] & 0xF]); }
    if (s.size() < 16) s = "A1B2C3D4E5F60708";
    return s;
}

void Sequencer::handle_ble_req(const CanFrame& f) {
    sca::BleConfig cfg;
    cfg.hash12 = (f.dlc == 8) ? bytes_to_hex16(f.data, f.dlc).substr(4) /* 16 → 마지막 12자리로 변환 */
        : std::string("E5F60708ABCD");            /* 예비값(12hex) */
    // 위 한 줄 설명: 기존엔 16-hex를 입력으로 받았지만, 내장코드가 UUID의 마지막 12자리만 받게 만들었음.
    // 만약 CAN에서 12바이트를 받을 수 있다면 그대로 넣어도 됨.
    if (cfg.hash12.size() != 12) {
        // 16자리에서 마지막 12자리 떼는 보수적 처리
        std::string h16 = (f.dlc == 8) ? bytes_to_hex16(f.data, f.dlc) : "A1B2C3D4E5F60708";
        cfg.hash12 = h16.substr(h16.size() - 12);
    }

    cfg.local_name = adv_name_;
    cfg.timeout_sec = ble_timeout_;
    cfg.expected_token = "ACCESS"; // 필요 시 CAN/TcuStore에서 받도록 변경 가능

    sca::BleResult br;
    sca::BlePeripheral ble;
    bool ok = ble.run(cfg, br);

    CanFrame rsp{}; rsp.id = ID_RSP_BLE; rsp.dlc = 1; rsp.data[0] = ok ? 0x00 : 0xFF;
    can_send("can0", rsp, 100);
}

void Sequencer::handle_nfc_req(const CanFrame&) {
    sca::NfcConfig cfg; cfg.poll_seconds = 5;
    sca::NfcResult nr;

    bool read_ok = sca::nfc_poll_once(cfg, nr);
    bool match = (read_ok && !nr.uid_hex.empty() && nr.uid_hex == expected_nfc_uid_);

    CanFrame rsp{}; rsp.id = ID_RSP_NFC; rsp.dlc = 1; rsp.data[0] = match ? 0x00 : 0xFF;
    can_send("can0", rsp, 100);
}
