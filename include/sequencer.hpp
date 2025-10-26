#pragma once
#include <cstdint>
#include <array>
#include <atomic>
#include <mutex>
#include <optional>
#include <string>

// can_api �� Ÿ��/�Լ�
#include "can_api.hpp"
#include "can_ids.hpp"
#include "canmessage.hpp"   // BCAN_ID_* ���� ���ǵǾ� �ִٰ� ����

// ���� �ܰ�
enum class AuthStep : uint8_t {
    Idle       = 0,
    WaitingTCU = 1,  // TCU ���� ���
    NFC        = 2,
    NFC_Wait   = 3,
    BLE        = 4,
    BLE_Wait   = 5,
    CAMERA     = 6,
    CAMERA_Wait= 7,
    Done       = 8
};

// ���� �÷��� (0=OK, 1=FAIL)
enum class AuthStateFlag : uint8_t {
    OK    = 0,
    FAIL  = 1
};

struct SequencerConfig {
    std::string can_channel = "can0";
    uint32_t    can_bitrate = 500000;

    // BLE ���� �Ķ����
    std::string ble_local_name = "SCA-CAR";
    int         ble_timeout_s  = 30;

    // NFC ���� Ÿ�Ӿƿ�
    int         nfc_timeout_s  = 5;
    std::string expected_uid_hex;

    // BLE ����/ç���� ���� �� �ӽ� fallback(�ʿ��)
    std::string ble_token_fallback = "ACCESS";
    
    std::string ble_uuid_last12 = "A1B2C3D4E5F6";
};

class Sequencer {
public:
    explicit Sequencer(const SequencerConfig& cfg);

    // CAN ���� ��(main���� can_subscribe �ݹ����� ȣ��)
    void on_can_rx(const CanFrame& f);

    // (main loop) ���¸ӽ� �� ����
    void tick();

private:
    // ����
    bool ok;
    SequencerConfig cfg_;
    std::atomic<AuthStep> step_{AuthStep::Idle};
    std::mutex m_;
    bool running_ = false;

    // TCU ���� ��밪
    std::array<uint8_t,8> expected_nfc_{};    bool have_expected_nfc_  = false;
    std::array<uint8_t,4> ble_sess_{};        bool have_ble_sess_      = false;
    std::array<uint8_t,4> ble_chal_{};        bool have_ble_chal_      = false;

    // ���� ��ƿ
    void reset_to_idle_();

    // �ܰ躰 ����
    void start_sequence_();               // DCU Ʈ���� ����
    void request_user_info_to_tcu_();     // 0x102
    bool perform_nfc_();                  // NFC ���� �� ��밪�� ��
    bool perform_ble_();                  // BLE ��ū ����/����

    // ���/���� ����
    void send_auth_state_(uint8_t step, AuthStateFlag flg);  // 0x103
    void send_auth_result_(bool ok);                         // 0x112
    void ack_user_info_(uint8_t index, uint8_t state);       // 0x111

    // ����
    static std::string to_hex_(const uint8_t* d, size_t n);
};
