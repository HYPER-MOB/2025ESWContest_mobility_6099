#include "sequencer.hpp"
#include <iostream>
#include <cctype>

static constexpr uint32_t ID_REQ_BLE = 0x100;
static constexpr uint32_t ID_REQ_NFC = 0x101;
static constexpr uint32_t ID_RSP_BLE = 0x110;
static constexpr uint32_t ID_RSP_NFC = 0x111;

void Sequencer::start() {
    // �ʿ��� �ʱ�ȭ�� ������ ���⼭
}

void Sequencer::stop() {
    // �ʿ��� ������ ������ ���⼭
}

void Sequencer::on_can_frame(const CanFrame& f) {
    
    if (f.id == ID_REQ_BLE) {
        handle_ble_req(f);
    }
    else if (f.id == ID_REQ_NFC) {
        handle_nfc_req(f);
    }
    else {
        // �ٸ� ID���� ���� ������Ρ�
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
    cfg.hash12 = (f.dlc == 8) ? bytes_to_hex16(f.data, f.dlc).substr(4) /* 16 �� ������ 12�ڸ��� ��ȯ */
        : std::string("E5F60708ABCD");            /* ����(12hex) */
    // �� �� �� ����: ������ 16-hex�� �Է����� �޾�����, �����ڵ尡 UUID�� ������ 12�ڸ��� �ް� �������.
    // ���� CAN���� 12����Ʈ�� ���� �� �ִٸ� �״�� �־ ��.
    if (cfg.hash12.size() != 12) {
        // 16�ڸ����� ������ 12�ڸ� ���� ������ ó��
        std::string h16 = (f.dlc == 8) ? bytes_to_hex16(f.data, f.dlc) : "A1B2C3D4E5F60708";
        cfg.hash12 = h16.substr(h16.size() - 12);
    }

    cfg.local_name = adv_name_;
    cfg.timeout_sec = ble_timeout_;
    cfg.expected_token = "ACCESS"; // �ʿ� �� CAN/TcuStore���� �޵��� ���� ����

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
