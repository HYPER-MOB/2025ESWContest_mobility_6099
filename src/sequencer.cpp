#include "sequencer.hpp"
#include <cstdio>
#include <cstring>
#include <iostream>

// �����(���� ����)
#include "nfc_adapter.hpp"
#include "ble_adapter.hpp"
#include "nfc/nfc_adapter.hpp"   // �� nfc_poll_uid ���� ��������

// �� BLE �������� ���� �ڵ忡�� ������ �Լ����� �״�� �����𡱸� ���ݴϴ�.
//   (���� ����/��ũ�� sca_ble_peripheral.cpp�� ���)
extern "C" bool sca_ble_advertise_and_wait(const char* uuid_last12,
                                           const char* local_name,
                                           int timeout_s,
                                           const char* expected_ascii);
// ===== ���� =====
std::string Sequencer::to_hex_(const uint8_t* d, size_t n) {
    static const char* k = "0123456789ABCDEF";
    std::string s; s.reserve(n*2);
    for (size_t i=0;i<n;++i){ s.push_back(k[d[i]>>4]); s.push_back(k[d[i]&0xF]); }
    return s;
}

// ===== CTOR =====
Sequencer::Sequencer(const SequencerConfig& cfg): cfg_(cfg) {
    reset_to_idle_();
}

// ===== ���� ��ƿ =====
void Sequencer::reset_to_idle_() {
    std::lock_guard<std::mutex> lk(m_);
    step_ = AuthStep::Idle;
    running_ = false;
    have_expected_nfc_ = false;
    have_ble_sess_ = have_ble_chal_ = false;
}

// ===== �ܰ� ���� =====
void Sequencer::start_sequence_() {
    if (running_) return;
    running_ = true;
    step_ = AuthStep::WaitingTCU;
    request_user_info_to_tcu_(); // TCU���� 0x102 ��û
    send_auth_state_(static_cast<uint8_t>(AuthStep::WaitingTCU), AuthStateFlag::OK);
}

// 0x102 SCA_TCU_USER_INFO_REQ �۽� (�÷��� 1)
void Sequencer::request_user_info_to_tcu_() {
    CanFrame f{}; f.id = BCAN_ID_SCA_TCU_USER_INFO_REQ; f.dlc = 1; f.data[0] = 1;
    can_send(cfg_.can_channel.c_str(), f, 0);
}

// ===== NFC ���� =====
bool Sequencer::perform_nfc_() {
    // TCU ��� NFC�� ���� �;� ��
    if (!have_expected_nfc_) {
        ack_user_info_(/*index=*/1, /*state=*/1); // ����
        send_auth_state_(static_cast<uint8_t>(AuthStep::NFC), AuthStateFlag::FAIL);
        return false;
    }

    std::array<uint8_t,8> read_uid{};
    const bool ok = nfc_poll_uid(read_uid, cfg_.nfc_timeout_s);
    if (!ok) {
        send_auth_state_(static_cast<uint8_t>(AuthStep::NFC), AuthStateFlag::FAIL);
        return false;
    }

    const bool match = (read_uid == expected_nfc_);
    send_auth_state_(static_cast<uint8_t>(AuthStep::NFC), match ? AuthStateFlag::OK : AuthStateFlag::FAIL);
    ack_user_info_(/*index=*/1, /*state=*/ match ? 0 : 1);
    return match;
}

// ===== BLE ���� =====
bool Sequencer::perform_ble_() {
    // ����/ç���� 4+4 ����Ʈ�� ���� �;� ��
    if (!have_ble_sess_ || !have_ble_chal_) {
        ack_user_info_(/*index=*/2, /*state=*/1); // ����
        send_auth_state_(static_cast<uint8_t>(AuthStep::BLE), AuthStateFlag::FAIL);
        return false;
    }

    // ��� ��ū�� ������||ç������ 8����Ʈ�� �� ASCII�� ��ȯ
    uint8_t cat[8];
    std::memcpy(cat,                 ble_sess_.data(), 4);
    std::memcpy(cat + 4,            ble_chal_.data(), 4);
    const std::string expected = to_hex_(cat, 8);

    // BLE ���� UUID ������ ��� 12�ڸ�: ���� 4����Ʈ(8 hex) + ç���� ���� 2����Ʈ(4 hex) ��
    // ������Ʈ �Ծ࿡ �°� ��ġ�� ��. �ӽ÷� ����(4B)=8hex + chal(����2B)=4hex => �� 12hex
    const std::string last12 = to_hex_(ble_sess_.data(), 4).substr(0,8) +
                               to_hex_(ble_chal_.data(), 4).substr(0,4);

    const bool matched = sca_ble_advertise_and_wait(
        last12,                // 12-hex (UUID ������ ���)
        cfg_.ble_local_name,   // ���� �̸�
        cfg_.ble_timeout_s,    // Ÿ�Ӿƿ�
        expected               // ��� ASCII(������ WriteValue�� ������ ��)
    );

    send_auth_state_(static_cast<uint8_t>(AuthStep::BLE), matched ? AuthStateFlag::OK : AuthStateFlag::FAIL);
    ack_user_info_(/*index=*/2, /*state=*/ matched ? 0 : 1);
    return matched;
}

// ===== CAN ���� ����ġ =====
void Sequencer::on_can_rx(const CanFrame& f) {
    switch (f.id) {
    // DCU�� ���� ����
    case BCAN_ID_DCU_SCA_USER_FACE_REQ: {
        start_sequence_();
        break;
    }
    // TCU �� NFC ��� UID (8����Ʈ)
    case BCAN_ID_TCU_SCA_USER_INFO_NFC: {
        if (f.dlc >= 8) {
            std::memcpy(expected_nfc_.data(), f.data, 8);
            have_expected_nfc_ = true;
            ack_user_info_(/*index=*/1, /*state=*/0);
        } else {
            ack_user_info_(/*index=*/1, /*state=*/1);
        }
        break;
    }
    // TCU �� BLE ����(4B)
    case BCAN_ID_TCU_SCA_USER_INFO_BLE_SESS: {
        if (f.dlc >= 4) {
            std::memcpy(ble_sess_.data(), f.data, 4);
            have_ble_sess_ = true;
            ack_user_info_(/*index=*/2, /*state=*/0);
        } else {
            ack_user_info_(/*index=*/2, /*state=*/1);
        }
        break;
    }
    // TCU �� BLE ç����(4B)
    case BCAN_ID_TCU_SCA_USER_INFO_BLE_CHAL: {
        if (f.dlc >= 4) {
            std::memcpy(ble_chal_.data(), f.data, 4);
            have_ble_chal_ = true;
            ack_user_info_(/*index=*/2, /*state=*/0);
        } else {
            ack_user_info_(/*index=*/2, /*state=*/1);
        }
        break;
    }
    default:
        break;
    }
}

// ===== tick: ���¸ӽ� �� ���� =====
void Sequencer::tick() {
    AuthStep cur = step_.load();
    if (!running_) return;

    switch (cur) {
    case AuthStep::WaitingTCU: {
        // ��� �����Ͱ� �� ���̸� NFC �ܰ��
        if (have_expected_nfc_) {
            step_ = AuthStep::NFC;
        }
        break;
    }
    case AuthStep::NFC: {
        bool ok = perform_nfc_();
        if (!ok) {
            send_auth_result_(false);
            reset_to_idle_();
        } else {
            step_ = AuthStep::BLE;
        }
        break;
    }
    case AuthStep::BLE: {
        bool ok = perform_ble_();
        send_auth_result_(ok);
        reset_to_idle_();
        break;
    }
    case AuthStep::Idle:
    case AuthStep::Done:
    default:
        break;
    }
}

// ===== ����/��� �۽� =====
void Sequencer::send_auth_state_(uint8_t step, AuthStateFlag flg) {
    CanFrame f{}; f.id = BCAN_ID_SCA_DCU_AUTH_STATE; f.dlc = 2;
    f.data[0] = step;
    f.data[1] = static_cast<uint8_t>(flg);
    can_send(cfg_.can_channel.c_str(), f, 0);
}

void Sequencer::send_auth_result_(bool ok) {
    CanFrame f{}; f.id = BCAN_ID_SCA_DCU_AUTH_RESULT; f.dlc = 1;
    f.data[0] = ok ? 0x00 : 0x01;
    can_send(cfg_.can_channel.c_str(), f, 0);
}

void Sequencer::ack_user_info_(uint8_t index, uint8_t state) {
    CanFrame f{}; f.id = BCAN_ID_SCA_TCU_USER_INFO_ACK; f.dlc = 2;
    f.data[0] = index; // 1:NFC, 2:BLE
    f.data[1] = state; // 0 OK, 1 ����/����
    can_send(cfg_.can_channel.c_str(), f, 0);
}
