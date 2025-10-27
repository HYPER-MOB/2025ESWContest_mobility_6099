#pragma once
#include <cstdint>
#include <array>
#include <atomic>
#include <mutex>
#include <optional>
#include <vector>
#include <string>

#include "can_api.hpp"
#include "can_ids.hpp"
#include "canmessage.hpp"

enum class AuthStep : uint8_t {
    Idle       = 0,
    WaitingTCU = 1,
    NFC        = 2,
    BLE        = 3,
    CAM        = 4,
    NFC_Wait   = 5,
    BLE_Wait   = 6,
    CAM_Setting= 7,
    CAM_Wait   = 8,
    Riding     = 9,
    Done       = 10
};

enum class AuthStateFlag : uint8_t {
    OK    = 0,
    FAIL  = 1
};
enum class eCamStatus
	{
		Wait =0,
		Action = 1,
		Ready = 2,
		Terminate =3,
		Result = 4,
		Error = 5
	};
struct SequencerConfig {
    std::string can_channel = "can0";
    uint32_t    can_bitrate = 500000;

    std::string ble_local_name = "SCA-CAR";
    int         ble_timeout_s  = 30;

    int         nfc_timeout_s  = 5;
    std::vector<std::pair<uint32_t, float>> cam_data_;

    std::string expected_uid_hex;

    std::string ble_token_fallback = "ACCESS";
    
    std::string ble_uuid_last12 = "A1B2C3D4E5F6";
};

class Sequencer {
public:
    explicit Sequencer(const SequencerConfig& cfg);

    void on_can_rx(const CanFrame& f);

    void tick();

private:
    bool ok;
    int retry_step;
    int cam_data_cnt;
    SequencerConfig cfg_;
    std::atomic<AuthStep> step_{AuthStep::Idle};
    std::mutex m_;
    bool running_ = false;

    std::array<uint8_t,8> expected_nfc_{};    bool have_expected_nfc_  = false;
    std::array<uint8_t,4> ble_sess_{};        bool have_ble_sess_      = false;
    std::array<uint8_t,4> ble_chal_{};        bool have_collected_cam_ = false;

    void reset_to_idle_();
    void start_sequence_();               
    void request_user_info_to_tcu_();   
    bool perform_nfc_();  
    bool perform_ble_();
    bool setting_cam_();
    bool perform_cam_(uint8_t* result);

    void send_auth_state_(uint8_t step, AuthStateFlag flg);  // 0x103
    void send_auth_result_(bool ok);                         // 0x112
    void ack_user_info_(uint8_t index, uint8_t state);       // 0x111

    static std::string to_hex_(const uint8_t* d, size_t n);
};
