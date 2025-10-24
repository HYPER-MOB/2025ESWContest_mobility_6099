#pragma once
#include <cstdint>
#include <array>
#include <atomic>
#include <mutex>
#include <optional>
#include <string>

struct CanFrame; // can_api가 제공하는 프레임 타입과 호환 전제

// 인증 단계
enum class AuthStep : uint8_t {
    Idle = 0,
    WaitingTCU = 1,  // TCU 정보 대기
    NFC = 2,
    BLE = 3,
    Done = 4
};

// 상태 플래그 (임의 정의: 0=OK, 1=FAIL)
enum class AuthStateFlag : uint8_t {
    OK = 0,
    FAIL = 1
};

struct SequencerConfig {
    std::string can_channel = "debug0";
    uint32_t    can_bitrate = 500000;

    // BLE 광고 파라미터 (내장 라이브러리 호출 시 사용)
    std::string ble_local_name = "SCA-CAR";
    int         ble_timeout_s = 30;

    // NFC 폴링 타임아웃
    int         nfc_timeout_s = 5;

    // BLE 토큰이 안 오면 fallback
    std::string ble_token_fallback = "ACCESS";
};

class Sequencer {
public:
    explicit Sequencer(const SequencerConfig& cfg);

    // CAN 수신 훅: main에서 can_subscribe 콜백/폴링으로 들어오는 프레임을 밀어넣는다.
    void on_can_rx(const CanFrame& f);

    // (main loop에서) IDLE 유지. 이벤트 오면 상태머신 한 단위 진행
    void tick();

private:
    // 내부 상태
    SequencerConfig cfg_;
    std::atomic<AuthStep> step_{ AuthStep::Idle };
    std::mutex m_;

    // TCU로부터 받은 기대값들 (옵션)
    std::array<uint8_t, 8> expected_nfc_{};
    bool have_expected_nfc_ = false;

    std::array<uint8_t, 4> ble_sess_{};
    std::array<uint8_t, 4> ble_chal_{};
    bool have_ble_sess_ = false;
    bool have_ble_chal_ = false;

    // 진행 중 플래그
    bool running_ = false;

    // 내부 유틸
    void reset_to_idle_();

    // 단계별 트리거
    void start_sequence_();                     // DCU 트리거 수신
    void request_user_info_to_tcu_();           // SCA_TCU_USER_INFO_REQ(0x102)
    bool perform_nfc_();                        // NFC 폴링 → 기대값과 비교
    bool perform_ble_();                        // BLE 내장 서버 띄우고 토큰 비교

    // 결과/상태 통지
    void send_auth_state_(uint8_t step, AuthStateFlag flg);
    void send_auth_result_(bool ok);
    void ack_user_info_(uint8_t index, uint8_t state);

    // 기대 토큰/UID 변환기
    static std::string to_hex_string_(const uint8_t* d, size_t n);
};
