#pragma once
#include <cstdint>
#include <array>
#include <atomic>
#include <mutex>
#include <optional>
#include <string>

struct CanFrame; // can_api�� �����ϴ� ������ Ÿ�԰� ȣȯ ����

// ���� �ܰ�
enum class AuthStep : uint8_t {
    Idle = 0,
    WaitingTCU = 1,  // TCU ���� ���
    NFC = 2,
    BLE = 3,
    Done = 4
};

// ���� �÷��� (���� ����: 0=OK, 1=FAIL)
enum class AuthStateFlag : uint8_t {
    OK = 0,
    FAIL = 1
};

struct SequencerConfig {
    std::string can_channel = "debug0";
    uint32_t    can_bitrate = 500000;

    // BLE ���� �Ķ���� (���� ���̺귯�� ȣ�� �� ���)
    std::string ble_local_name = "SCA-CAR";
    int         ble_timeout_s = 30;

    // NFC ���� Ÿ�Ӿƿ�
    int         nfc_timeout_s = 5;

    // BLE ��ū�� �� ���� fallback
    std::string ble_token_fallback = "ACCESS";
};

class Sequencer {
public:
    explicit Sequencer(const SequencerConfig& cfg);

    // CAN ���� ��: main���� can_subscribe �ݹ�/�������� ������ �������� �о�ִ´�.
    void on_can_rx(const CanFrame& f);

    // (main loop����) IDLE ����. �̺�Ʈ ���� ���¸ӽ� �� ���� ����
    void tick();

private:
    // ���� ����
    SequencerConfig cfg_;
    std::atomic<AuthStep> step_{ AuthStep::Idle };
    std::mutex m_;

    // TCU�κ��� ���� ��밪�� (�ɼ�)
    std::array<uint8_t, 8> expected_nfc_{};
    bool have_expected_nfc_ = false;

    std::array<uint8_t, 4> ble_sess_{};
    std::array<uint8_t, 4> ble_chal_{};
    bool have_ble_sess_ = false;
    bool have_ble_chal_ = false;

    // ���� �� �÷���
    bool running_ = false;

    // ���� ��ƿ
    void reset_to_idle_();

    // �ܰ躰 Ʈ����
    void start_sequence_();                     // DCU Ʈ���� ����
    void request_user_info_to_tcu_();           // SCA_TCU_USER_INFO_REQ(0x102)
    bool perform_nfc_();                        // NFC ���� �� ��밪�� ��
    bool perform_ble_();                        // BLE ���� ���� ���� ��ū ��

    // ���/���� ����
    void send_auth_state_(uint8_t step, AuthStateFlag flg);
    void send_auth_result_(bool ok);
    void ack_user_info_(uint8_t index, uint8_t state);

    // ��� ��ū/UID ��ȯ��
    static std::string to_hex_string_(const uint8_t* d, size_t n);
};
