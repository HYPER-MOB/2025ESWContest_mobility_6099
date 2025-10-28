#include "sequencer.hpp"
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <iostream>
#include "sca_ble_peripheral.hpp"
#include "nfc_reader.hpp"
#include "camera_adapter.hpp"
#include <array>
#include <cstdint>

using sca::cam_initial_;
using sca::cam_data_setting_;
using sca::cam_start_;
using sca::cam_Terminate_;
using sca::cam_authenticating_;
using sca::cam_authenticating_drive_;

uint32_t bswap32(uint32_t v) {
    return ((v & 0x000000FFu) << 24) |
        ((v & 0x0000FF00u) << 8) |
        ((v & 0x00FF0000u) >> 8) |
        ((v & 0xFF000000u) >> 24);
}
static bool bytes_from_hex_relaxed(const std::string& in, uint8_t* out, int max_len, int& written) {
    written = 0;
    if (!out || max_len <= 0) return false;

    size_t i = 0;

    const int need = (int)(in.size() / 2);
    if (need > max_len) return false;

    auto nib = [](char c)->int {
        if ('0' <= c && c <= '9') return c - '0';
        if ('A' <= c && c <= 'F') return 10 + (c - 'A');
        return -1;
        };

    for (int k = 0; k < need; ++k) {
        int hi = nib(in[2 * k + 0]);
        int lo = nib(in[2 * k + 1]);
        if (hi < 0 || lo < 0) return false;
        out[k] = (uint8_t)((hi << 4) | lo);
    }
    written = need;
    return true;
}
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

extern "C" bool nfc_read_uid(uint8_t* out, int len, int timeout_s) {
    if (!out || len <= 0) return false;
    std::memset(out, 0, (size_t)len);

    sca::NfcConfig cfg{};
    cfg.poll_seconds = (timeout_s > 0 ? timeout_s : 5);
    sca::NfcResult res{};
    if (!sca::nfc_poll_once(cfg, res)) return false;
    if (!res.ok || res.uid_hex.empty()) return false;
    
    int written=0;
    std::string str =res.uid_hex.substr(res.use_apdu?16:0,res.uid_hex.size());
    bytes_from_hex_relaxed(str, out, sizeof(out), written);
    std::printf("ADPU :");
    for (int i = 0; i < len; i++)
    {
        std::printf("%02X", out[i]);
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
    have_ble_sess_ = false;
    have_collected_cam_ = false;
}
void Sequencer::start_sequence_() {
    if (running_) return;
    cam_data_cnt = 0;
    running_ = true;
    step_ = AuthStep::WaitingTCU;
    request_user_info_to_tcu_();
    send_auth_state_(static_cast<uint8_t>(AuthStep::WaitingTCU), AuthStateFlag::OK);
}


bool Sequencer::perform_nfc_() {
    uint8_t buf[8] = {0};
    if (!nfc_read_uid(buf, 8, cfg_.nfc_timeout_s)) return false;

    for (int i = 0; i < 8; ++i) {
        if (buf[i] != expected_nfc_[i]) return false;
    }
    return true;
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
bool Sequencer::setting_cam_(bool type) {
    bool ok = cam_initial_(type);
    if (!ok) return false;
    if(type)cam_data_setting_(cam_data_.data(), cam_data_cnt);
    return true;
}

bool Sequencer::perform_cam_(uint8_t* result) {
    bool ok_flag = false;
    uint8_t st  = cam_authenticating_(&ok_flag);
    if (result) *result = st;
    return ok_flag;
}
void Sequencer::on_can_rx(const CanFrame& f) {
    for(int i=0;i<f.dlc;i++)
    {
        std::printf("%02X ",f.data[i]);
    }
    std::printf("\n");
    switch (f.id) {
    case PCAN_ID_DCU_SCA_USER_FACE_REQ: {
        start_sequence_();
        break;
    }
    case PCAN_ID_TCU_SCA_USER_INFO_NFC: {
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
    case PCAN_ID_TCU_SCA_USER_INFO_BLE_SESS: {
        if (f.dlc >= 4) {
            std::memcpy(ble_sess_.data(), f.data+ sizeof(uint8_t) * 2, 6);
            have_ble_sess_ = true;
            ack_user_info_(/*index=*/2, /*state=*/0);
        } else {
            ack_user_info_(/*index=*/2, /*state=*/1);
        }
        break;
    }
    case PCAN_ID_TCU_SCA_USER_INFO: {
        if (f.dlc == 8) {
        uint32_t v;
        std::memcpy(&v, f.data, 4);

            if (v == 0xFFFFFFFF) {
                std::printf("[Data]");
                have_collected_cam_ = true;
                cam_data_[cam_data_cnt].first = v;
                cam_data_[cam_data_cnt].second = 0;
                cam_data_cnt++;
            ack_user_info_(/*index=*/3, /*state=*/0);
                break;
            }
            else
            {
                uint32_t idx;
                float data;
                std::memcpy(&idx, f.data, 4);
                std::memcpy(&data, f.data + sizeof(uint8_t) * 4, 4);
                cam_data_[cam_data_cnt].first = idx;
                cam_data_[cam_data_cnt].second = data;
                cam_data_cnt++;
            }
            ack_user_info_(/*index=*/3, /*state=*/0);
        }
        else {
            ack_user_info_(/*index=*/3, /*state=*/1);
        }
        break;
    }
    case PCAN_ID_DCU_SCA_DRIVE_STATUS: {
        std::printf("[Drive] :%d",f.data[0]);
        driving = f.data[0] > 0;
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
            send_auth_result_(false);
            reset_to_idle_();
        } else {
            std::printf("[NFC] End\n");
            send_auth_state_(static_cast<uint8_t>(AuthStep::NFC), AuthStateFlag::OK);
            send_auth_result_(true);
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
            send_auth_result_(false);
            reset_to_idle_();
        }
        else{
            std::printf("[BLE] End\n");
            send_auth_state_(static_cast<uint8_t>(AuthStep::BLE), AuthStateFlag::OK);
            send_auth_result_(true);
            retry_step = 0;
            step_ = AuthStep::CAM;
        }
        break;
    }
     case AuthStep::CAM: {
         if (have_collected_cam_) {
             std::printf("[CAM] Init\n");
             ok = setting_cam_(true);
             if(ok)
                step_ = AuthStep::CAM_Wait;
            else if(retry_step<3)
                retry_step++;
            else{
                std::printf("[CAM] Program Issue\n");
                send_auth_state_(static_cast<uint8_t>(AuthStep::CAM), AuthStateFlag::FAIL);
                send_auth_result_(FALSE);
                reset_to_idle_();
            }

         }
         break;
     }
     case AuthStep::CAM_Wait: {
            uint8_t result;
            ok = perform_cam_(&result);
            
            switch (result)
            {
            case 1/*Ready*/:
                std::printf("[CAM] Start\n");
                cam_start_();
                break;
            case 2/*Terminate*/:
                std::printf("[CAM] End\n");
                send_auth_result_(false);
                reset_to_idle_();
                break;
            case 3/*Result*/:
                std::printf("[CAM] Result : %d\n",ok);
                send_auth_state_(static_cast<uint8_t>(AuthStep::CAM), ok?AuthStateFlag::OK:AuthStateFlag::FAIL);
                send_auth_result_(ok);
                cam_Terminate_();
                break;
            case 4/*Error*/:
                cam_Terminate_();
                std::printf("[CAM] Fail\n");
                send_auth_state_(static_cast<uint8_t>(AuthStep::CAM), AuthStateFlag::FAIL);
                send_auth_result_(false);
                reset_to_idle_();
                break;
            
            default:
                break;
            }

         }
         break;
     case AuthStep::Drive:
     {
         std::printf("[Drive] Init\n");
         ok = setting_cam_(false);
        if (ok)
            step_ = AuthStep::Driving;
        else if (retry_step < 3)
            retry_step++;
        else {
            std::printf("[Drive] Program Issue\n");
            reset_to_idle_();
        }
         break;
     }break;
     case AuthStep::Driving:
     {
         if (!driving) {
             cam_Terminate_();
             step_ = AuthStep::Idle;
         }
         ok = cam_authenticating_drive_();
         if (ok) {
             send_sleep_check();
             cam_Terminate_();
             step_ = AuthStep::Idle;
         }
     }break;
    case AuthStep::Idle:
        if (driving)
            step_ = AuthStep::Drive;
        break;
    case AuthStep::Done:
    default:
        break;
    }
}

void Sequencer::send_auth_state_(uint8_t step, AuthStateFlag flg) {
    CanFrame f{}; f.id = PCAN_ID_SCA_DCU_AUTH_STATE; f.dlc = 2;
    f.data[0] = step;
    f.data[1] = static_cast<uint8_t>(flg);
    can_send(cfg_.can_channel.c_str(), f, 0);
}

void Sequencer::send_auth_result_(bool ok) {
    CanFrame f{}; f.id = PCAN_ID_SCA_DCU_AUTH_RESULT; f.dlc = 8;
    memset(f.data, 0, sizeof(f.data));
    f.data[0] = ok ? 0x00 : 0x01;
    can_send(cfg_.can_channel.c_str(), f, 0);
}

void Sequencer::send_sleep_check() {
    CanFrame f{}; f.id = PCAN_ID_SCA_DCU_DRIVER_EVENT; f.dlc = 1;
    f.data[0] = 0;
    can_send(cfg_.can_channel.c_str(), f, 0);
}

void Sequencer::ack_user_info_(uint8_t index, uint8_t state) {
    
    CanFrame f{}; f.id = PCAN_ID_SCA_TCU_USER_INFO_ACK; f.dlc = 2;
    f.data[0] = index; // 1:NFC, 2:BLE
    f.data[1] = state; // 0 OK, 1 
    can_send(cfg_.can_channel.c_str(), f, 0);
}

void Sequencer::request_user_info_to_tcu_() {
    CanFrame f{}; f.id = PCAN_ID_SCA_TCU_USER_INFO_REQ; f.dlc = 1; f.data[0] = 1;
    can_send(cfg_.can_channel.c_str(), f, 0);
}