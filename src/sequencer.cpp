#include "sequencer.hpp"
#include "can_ids.hpp"

// can_api �������̽�
#include "can_api.hpp"     // can_init, can_open, can_send, can_recv ��
#include "canmessage.hpp"  // (�ʿ�� �޽��� ���ڴ�/���ڴ� ��� ����)

#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>

// BLE/NFC ���� ���̺귯��
#include "../ble/sca_ble_peripheral.hpp"
#include "../nfc/nfc_reader.hpp"

// ---- �����: CAN ������ ���� (can_api�� ����) ----
struct CanFrame {
    uint32_t id = 0;
    uint8_t  dlc = 0;
    uint8_t  data[8] = { 0 };
};

static CanFrame mk_frame(uint32_t id, const uint8_t* d, uint8_t n) {
    CanFrame f; f.id = id; f.dlc = (n > 8 ? 8 : n);
    if (d && n) std::memcpy(f.data, d, f.dlc);
    return f;
}

// ---- Sequencer ----
Sequencer::Sequencer(const SequencerConfig& cfg) : cfg_(cfg) {
    reset_to_idle_();
}

void Sequencer::reset_to_idle_() {
    std::lock_guard<std::mutex> lk(m_);
    running_ = false;
    step_.store(AuthStep::Idle);
    have_expected_nfc_ = false;
    have_ble_sess_ = false;
    have_ble_chal_ = false;
    expected_nfc_.fill(0);
    ble_sess_.fill(0);
    ble_chal_.fill(0);
}

// hex helper
std::string Sequencer::to_hex_string_(const uint8_t* d, size_t n) {
    static const char* k = "0123456789ABCDEF";
    std::string s; s.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) { s.push_back(k[d[i] >> 4]); s.push_back(k[d[i] & 0xF]); }
    return s;
}

void Sequencer::on_can_rx(const CanFrame& f) {
    // �̺�Ʈ ����ġ
    if (f.id == BCAN_ID_DCU_SCA_USER_FACE_REQ) {
        start_sequence_();
        return;
    }
    if (f.id == BCAN_ID_TCU_SCA_USER_INFO_NFC) {
        // 8����Ʈ ��� UID ����
        {
            std::lock_guard<std::mutex> lk(m_);
            std::memcpy(expected_nfc_.data(), f.data, 8);
            have_expected_nfc_ = true;
        }
        // ACK index=1(����), state=0(OK)
        ack_user_info_(/*index*/1, /*state*/0);
        return;
    }
    if (f.id == BCAN_ID_TCU_SCA_USER_INFO_BLE_SESS) {
        std::lock_guard<std::mutex> lk(m_);
        std::memcpy(ble_sess_.data(), f.data, 4);
        have_ble_sess_ = true;
        ack_user_info_(/*index*/2, /*state*/0);
        return;
    }
    if (f.id == BCAN_ID_TCU_SCA_USER_INFO_BLE_CHAL) {
        std::lock_guard<std::mutex> lk(m_);
        std::memcpy(ble_chal_.data(), f.data, 4);
        have_ble_chal_ = true;
        ack_user_info_(/*index*/3, /*state*/0);
        return;
    }
    // ��Ÿ ID�� �� �ܰ迡�� ����
}

void Sequencer::start_sequence_() {
    std::lock_guard<std::mutex> lk(m_);
    if (running_) {
        // �̹� ���� ������ ���� (���ϸ� �ߴ� �� ��������� �ٲ㵵 ��)
        return;
    }
    running_ = true;
    step_.store(AuthStep::WaitingTCU);

    // 1�ܰ� ���� �˸�
    send_auth_state_(/*step*/1, AuthStateFlag::OK);

    // TCU�� ����� ���� ��û
    request_user_info_to_tcu_();
}

void Sequencer::request_user_info_to_tcu_() {
    uint8_t flag = 1; // �ǹ� ����
    CanFrame f = mk_frame(BCAN_ID_SCA_TCU_USER_INFO_REQ, &flag, 1);
    can_send(cfg_.can_channel.c_str(), f, 0);
}

// NFC ����
bool Sequencer::perform_nfc_() {
    step_.store(AuthStep::NFC);

    // ������� ���� ��� UID?
    std::array<uint8_t, 8> expected{};
    {
        std::lock_guard<std::mutex> lk(m_);
        if (!have_expected_nfc_) {
            // ��밪 ������ ���з� ����
            send_auth_state_(/*step*/1, AuthStateFlag::FAIL);
            return false;
        }
        expected = expected_nfc_;
    }

    // NFC ����
    std::string observed_uid = nfc::poll_uid_once(cfg_.nfc_timeout_s); // ������ "" ����
    if (observed_uid.empty()) {
        send_auth_state_(/*step*/1, AuthStateFlag::FAIL);
        return false;
    }
    // ��밪(8B)�� �� (�� ����: HEX ���ڿ� ���ϼ�)
    std::string expected_hex = to_hex_string_(expected.data(), expected.size());
    bool ok = (observed_uid == expected_hex);

    send_auth_state_(/*step*/1, ok ? AuthStateFlag::OK : AuthStateFlag::FAIL);
    return ok;
}

// BLE ����
bool Sequencer::perform_ble_() {
    step_.store(AuthStep::BLE);

    // ��� ��ū ����: �켱 TCU ����(sess+chal) 8����Ʈ hex, ������ fallback
    std::string token;
    {
        std::lock_guard<std::mutex> lk(m_);
        if (have_ble_sess_ || have_ble_chal_) {
            uint8_t buf[8] = { 0 };
            std::memcpy(buf, ble_sess_.data(), 4);
            std::memcpy(buf + 4, ble_chal_.data(), 4);
            token = to_hex_string_(buf, 8);
        }
        else {
            token = cfg_.ble_token_fallback; // "ACCESS"
        }
    }

    // ���� UUID�� last 12-hex�� �ƹ��ų�(��: sess+chal) Ȥ�� ����
    std::string last12 = "ABCDEF123456";
    if (have_ble_sess_) {
        // sess 4B �� 8hex + "1234" ������ (�浹 �����ӽ�)
        last12 = to_hex_string_(ble_sess_.data(), 4) + "12";
        last12 = last12.substr(0, 12);
    }

    bool ok = ble::run_peripheral_once(
        /*uuid_last12*/ last12,
        /*local_name*/  cfg_.ble_local_name,
        /*timeout_s*/   cfg_.ble_timeout_s,
        /*expected_token*/ token,
        /*require_encrypt*/ false
    );

    send_auth_state_(/*step*/2, ok ? AuthStateFlag::OK : AuthStateFlag::FAIL);
    return ok;
}

void Sequencer::send_auth_state_(uint8_t step, AuthStateFlag flg) {
    uint8_t payload[2] = { step, (uint8_t)flg };
    CanFrame f = mk_frame(BCAN_ID_SCA_DCU_AUTH_STATE, payload, 2);
    can_send(cfg_.can_channel.c_str(), f, 0);
}

void Sequencer::send_auth_result_(bool ok) {
    // flag: 0=OK, 1=FAIL. user_id�� ������ "USER001" ����(�ִ� 7B)
    uint8_t payload[8] = { 0 };
    payload[0] = ok ? 0x00 : 0x01;

    const char* uid = "USER001";
    size_t L = std::min<size_t>(7, std::strlen(uid));
    std::memcpy(&payload[1], uid, L);

    CanFrame f = mk_frame(BCAN_ID_SCA_DCU_AUTH_RESULT, payload, 8);
    can_send(cfg_.can_channel.c_str(), f, 0);

    // �ʿ� �� ADD(0x113)�� �̾ ���� ����
}

void Sequencer::ack_user_info_(uint8_t index, uint8_t state) {
    uint8_t d[2] = { index, state };
    CanFrame f = mk_frame(BCAN_ID_SCA_TCU_USER_INFO_ACK, d, 2);
    can_send(cfg_.can_channel.c_str(), f, 0);
}

void Sequencer::tick() {
    // ����� �� (Ʈ����) �� TCU info ��� �� NFC �� (���� ��) BLE �� ��� �� ��⺹�͡�
    AuthStep s = step_.load();

    if (s == AuthStep::WaitingTCU) {
        // NFC ��밪 ���ŵǸ� NFC ����
        bool haveNfc = false;
        {
            std::lock_guard<std::mutex> lk(m_);
            haveNfc = have_expected_nfc_;
        }
        if (haveNfc) {
            bool nfc_ok = perform_nfc_();
            if (!nfc_ok) {
                send_auth_result_(false);
                reset_to_idle_();
                return;
            }
            // NFC ���� �� BLE �õ�
            bool ble_ok = perform_ble_();
            send_auth_result_(ble_ok);
            reset_to_idle_();
        }
    }
    // Idle/Done�� �ܺ� �̺�Ʈ ��ٸ�
}
