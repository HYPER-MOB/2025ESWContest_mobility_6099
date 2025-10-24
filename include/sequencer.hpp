#pragma once
#include <cstdint>
#include <array>
#include <atomic>
#include <mutex>
#include <optional>
#include <string>

// can_api 쪽 타입/함수
#include "can_api.hpp"
#include "canmessage.hpp"   // BCAN_ID_* 들이 정의되어 있다고 가정

// 인증 단계
enum class AuthStep : uint8_t {
    Idle       = 0,
    WaitingTCU = 1,  // TCU 정보 대기
    NFC        = 2,
    BLE        = 3,
    Done       = 4
};

// 상태 플래그 (0=OK, 1=FAIL)
enum class AuthStateFlag : uint8_t {
    OK    = 0,
    FAIL  = 1
};

struct SequencerConfig {
    std::string can_channel = "debug0";
    uint32_t    can_bitrate = 500000;

    // BLE 광고 파라미터
    std::string ble_local_name = "SCA-CAR";
    int         ble_timeout_s  = 30;

    // NFC 폴링 타임아웃
    int         nfc_timeout_s  = 5;

    // BLE 세션/챌린지 수신 전 임시 fallback(필요시)
    std::string ble_token_fallback = "ACCESS";
};

class Sequencer {
public:
    explicit Sequencer(const SequencerConfig& cfg);

    // CAN 수신 훅(main에서 can_subscribe 콜백으로 호출)
    void on_can_rx(const CanFrame& f);

    // (main loop) 상태머신 한 스텝
    void tick();

private:
    // 상태
    SequencerConfig cfg_;
    std::atomic<AuthStep> step_{AuthStep::Idle};
    std::mutex m_;
    bool running_ = false;

    // TCU 제공 기대값
    std::array<uint8_t,8> expected_nfc_{};    bool have_expected_nfc_  = false;
    std::array<uint8_t,4> ble_sess_{};        bool have_ble_sess_      = false;
    std::array<uint8_t,4> ble_chal_{};        bool have_ble_chal_      = false;

    // 내부 유틸
    void reset_to_idle_();

    // 단계별 동작
    void start_sequence_();               // DCU 트리거 수신
    void request_user_info_to_tcu_();     // 0x102
    bool perform_nfc_();                  // NFC 폴링 → 기대값과 비교
    bool perform_ble_();                  // BLE 토큰 수신/검증

    // 결과/상태 통지
    void send_auth_state_(uint8_t step, AuthStateFlag flg);  // 0x103
    void send_auth_result_(bool ok);                         // 0x112
    void ack_user_info_(uint8_t index, uint8_t state);       // 0x111

    // 헬퍼
    static std::string to_hex_(const uint8_t* d, size_t n);
};
