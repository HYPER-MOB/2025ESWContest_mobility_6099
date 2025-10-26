#include "sequencer.hpp"
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <iostream>
#include "sca_ble_peripheral.hpp"
#include "nfc_reader.hpp"
#include <array>
#include <cstdint>
extern "C" bool sca_ble_advertise_and_wait(std::string uuid_last12,
    std::string local_name,
    int timeout_s) {
    sca::BleConfig cfg{};
    cfg.hash12 = uuid_last12; 
    cfg.local_name = local_name[0]>0 ? local_name : "SCA-CAR";
    cfg.timeout_sec = timeout_s > 0 ? timeout_s : 20;
    cfg.require_encrypt = false;

    sca::BlePeripheral p;
    sca::BleResult     out{};
    return p.run(cfg, out);
}

static bool bytes_from_hex_relaxed(const std::string& in, uint8_t* out, int max_len, int& written) {
    written = 0;
    if (!out || max_len <= 0) return false;

    // 정규화: 공백/하이픈/콜론 제거 + 0x prefix 제거 + 대문자화
    size_t i = 0;

    const int need = (int)(in.size()/2);
    if (need > max_len) return false;

    auto nib = [](char c)->int {
        if ('0'<=c && c<='9') return c - '0';
        if ('A'<=c && c<='F') return 10 + (c - 'A');
        return -1;
    };

    for (int k=0; k<need; ++k) {
        int hi = nib(in[2*k+0]);
        int lo = nib(in[2*k+1]);
        if (hi<0 || lo<0) return false;
        out[k] = (uint8_t)((hi<<4)|lo);
}
    written = need;
    return true;
}

extern "C" bool nfc_read_uid(uint8_t* out, int len, int timeout_s) {
    if (!out || len <= 0) return false;
    std::memset(out, 0, (size_t)len);

    sca::NfcConfig cfg{};
    cfg.poll_seconds = (timeout_s > 0 ? timeout_s : 5);
    sca::NfcResult res{};
    if (!sca::nfc_poll_once(cfg, res)) return false;
    if (!res.ok || res.uid_hex.empty()) return false;
    
    int written=0;
    
    bytes_from_hex_relaxed((res.use_apdu? res.uid_hex : res.uid_hex+6), out, sizeof(out), written);
    for (int i = 0; i < len; i++)
    {
        std::printf("%02X ", out[i]);
    }
    std::printf("\n");
    return (written > 0);
}

std::string Sequencer::to_hex_(const uint8_t* d, size_t n) {
    static const char* k = "0123456789ABCDEF";
    std::string s;
    s.resize(n*2);
    for (size_t i=0;i<n;++i){
        s[i*2]=(k[d[i]>>4]);
        s[i*2+1]=(k[d[i]&0xF]);
    }
    std::reverse(s.begin(),s.end());
    return s;
}
Sequencer::Sequencer(const SequencerConfig& cfg): cfg_(cfg) {
    reset_to_idle_();
}

void Sequencer::reset_to_idle_() {
    std::lock_guard<std::mutex> lk(m_);
    step_ = AuthStep::Idle;
    running_ = false;
    have_expected_nfc_ = false;
    have_ble_sess_ = have_ble_chal_ = false;
}
void Sequencer::start_sequence_() {
    if (running_) return;
    running_ = true;
    step_ = AuthStep::WaitingTCU;
    request_user_info_to_tcu_();
    send_auth_state_(static_cast<uint8_t>(AuthStep::WaitingTCU), AuthStateFlag::OK);
}


bool Sequencer::perform_nfc_() {
    uint8_t* read_uid=new uint8_t[8];
    const bool ok = nfc_read_uid(read_uid,8,cfg_.nfc_timeout_s);
    if (!ok) {
        return false;
    }
    bool match = (true);
    int i =0;
    while(read_uid[i]!=0)
    {
        if(read_uid[i]!=expected_nfc_[i])
        {
            match =false;
            break;
        }
        i++;
    }
    return match;
}

bool Sequencer::perform_ble_() {
    const std::string last12 = to_hex_(ble_sess_.data(), 6);
    const bool matched = sca_ble_advertise_and_wait(
        last12,
        cfg_.ble_local_name,
        cfg_.ble_timeout_s
    );

    return matched;
}
bool Sequencer::perform_cam_() {
    return true;
}
void Sequencer::on_can_rx(const CanFrame& f) {
    std::printf("[CAN] id: %d dlc:%d data:",f.id,f.dlc);
    for(int i=0;i<f.dlc;i++)
    {
        std::printf("%02X",f.data[i]);
    }
    std::printf("\n");
    switch (f.id) {
    case BCAN_ID_DCU_SCA_USER_FACE_REQ: {
        start_sequence_();
        break;
    }
    case BCAN_ID_TCU_SCA_USER_INFO_NFC: {
        if (f.dlc >= 4) {
            for(int i=0;i<f.dlc;i++)
            {
                expected_nfc_[f.dlc-i-1]=f.data[i];
            }
            have_expected_nfc_ = true;
            ack_user_info_(/*index=*/1, /*state=*/0);
        } else {
            ack_user_info_(/*index=*/1, /*state=*/1);
        }
        break;
    }
    case BCAN_ID_TCU_SCA_USER_INFO_BLE_SESS: {
        if (f.dlc >= 4) {
            std::memcpy(ble_sess_.data(), f.data+sizeof(uint8_t)*2, 6);
            have_ble_sess_ = true;
            ack_user_info_(/*index=*/2, /*state=*/0);
        } else {
            ack_user_info_(/*index=*/2, /*state=*/1);
        }
        break;
    }
    case BCAN_ID_TCU_SCA_USER_INFO: {
        // 카메라 데이터 처리
        if(1)have_collected_cam_;
        if (f.dlc >= 4) {
            ack_user_info_(/*index=*/2, /*state=*/0);
        }
        else {
            ack_user_info_(/*index=*/2, /*state=*/1);
        }
        break;
    }
    default:
        break;
    }
}
void Sequencer::tick() {
    AuthStep cur = step_.load();
    if (!running_) return;
    switch (cur) {
    case AuthStep::WaitingTCU: {
        step_ = AuthStep::NFC;
        break;
    }
    case AuthStep::NFC: {
        if (have_expected_nfc_)
        {
            std::printf("[NFC START]\n");
            ok = perform_nfc_();
            step_ = AuthStep::NFC_Wait;
        }
        break;
    }
    case AuthStep::NFC_Wait: {
        if (!ok) {
            std::printf("[NFC] Fail\n");
            send_auth_state_(static_cast<uint8_t>(AuthStep::NFC), AuthStateFlag::FAIL);
            send_auth_result_(FALSE);
            reset_to_idle_();
        } else {
            std::printf("[NFC] End\n");
            send_auth_state_(static_cast<uint8_t>(AuthStep::NFC), AuthStateFlag::OK);
            send_auth_result_(TRUE);
            step_ = AuthStep::BLE;
        }
        break;
    }
    case AuthStep::BLE: {
        if (have_ble_sess_) {
            std::printf("[BLE] START\n");
            ok = perform_ble_();
            step_ = AuthStep::BLE_Wait;
        }
        break;
    }
     case AuthStep::BLE_Wait: {
        if (!ok) {
            std::printf("[BLE] Fail\n");
            send_auth_state_(static_cast<uint8_t>(AuthStep::BLE), AuthStateFlag::FAIL);
            send_auth_result_(FALSE);
            reset_to_idle_();
        }
        else{
            std::printf("[BLE] End\n");
            send_auth_state_(static_cast<uint8_t>(AuthStep::BLE), AuthStateFlag::OK);
            send_auth_result_(TRUE);
            step_ = AuthStep::CAM_Wait;
        }
        break;
    }
     case AuthStep::CAM: {
         if (have_collected_cam_) {
             std::printf("[CAM] START\n");
             ok = perform_cam_();
             step_ = AuthStep::CAM_Wait;
         }
         break;
     }
     case AuthStep::CAM_Wait: {
         if (!ok) {
             std::printf("[CAM] Fail\n");
             send_auth_state_(static_cast<uint8_t>(AuthStep::CAM), AuthStateFlag::FAIL);
         }
         else {
             std::printf("[CAM] End\n");
             send_auth_state_(static_cast<uint8_t>(AuthStep::CAM), AuthStateFlag::OK);
         }
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

void Sequencer::send_auth_state_(uint8_t step, AuthStateFlag flg) {
    CanFrame f{}; f.id = BCAN_ID_SCA_DCU_AUTH_STATE; f.dlc = 2;
    f.data[0] = step;
    f.data[1] = static_cast<uint8_t>(flg);
    can_send(cfg_.can_channel.c_str(), f, 0);
}

void Sequencer::send_auth_result_(bool ok) {
    CanFrame f{}; f.id = BCAN_ID_SCA_DCU_AUTH_RESULT; f.dlc = 8;
    memset(f.data, 0, sizeof(f.data));
    f.data[0] = ok ? 0x00 : 0x01;
    can_send(cfg_.can_channel.c_str(), f, 0);
}

void Sequencer::ack_user_info_(uint8_t index, uint8_t state) {
    
    std::printf("[ACK]\n");
    CanFrame f{}; f.id = BCAN_ID_SCA_TCU_USER_INFO_ACK; f.dlc = 2;
    f.data[0] = index; // 1:NFC, 2:BLE
    f.data[1] = state; // 0 OK, 1 
    can_send(cfg_.can_channel.c_str(), f, 0);
}

void Sequencer::request_user_info_to_tcu_() {
    CanFrame f{}; f.id = BCAN_ID_SCA_TCU_USER_INFO_REQ; f.dlc = 1; f.data[0] = 1;
    can_send(cfg_.can_channel.c_str(), f, 0);
}