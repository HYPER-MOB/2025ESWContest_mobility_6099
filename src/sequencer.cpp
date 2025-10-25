#include "sequencer.hpp"
#include <cstdio>
#include <cstring>
#include <iostream>
#include "sca_ble_peripheral.hpp"  // nfc_poll_uid 제공
#include "nfc_reader.hpp"  // nfc_poll_uid 제공
#include <array>
#include <cstdint>

// ★ BLE 진입점은 기존 코드에서 제공된 함수명을 그대로 “선언”만 해줍니다.
//   (실제 구현/링크는 sca_ble_peripheral.cpp가 담당)
extern "C" bool sca_ble_advertise_and_wait(std::string uuid_last12,
    std::string local_name,
    int timeout_s,
    std::string expected_ascii) {
    sca::BleConfig cfg{};
    cfg.hash12 = uuid_last12;       // 12-hex (대문자 가정; 내부에서 대문자화)
    cfg.local_name = local_name[0]>0 ? local_name : "SCA-CAR";
    cfg.timeout_sec = timeout_s > 0 ? timeout_s : 20;
    cfg.expected_token = expected_ascii[0]>0 ? expected_ascii : "";
    cfg.require_encrypt = false;             // 정책 필요시 true

    sca::BlePeripheral p;
    sca::BleResult     out{};
    return p.run(cfg, out);
}

static bool hex_to_bytes_strict(const std::string& hex, uint8_t* out, int max_len, int& written) {
    written = 0;
    if (!out || max_len <= 0) return false;
    if (hex.size() < 2) return false;
    if (hex.size() % 2 != 0) return false; // 짝수 글자만 허용

    auto nib = [](char c)->int{
        if ('0'<=c && c<='9') return c-'0';
        unsigned char u = (unsigned char)std::toupper(c);
        if ('A'<=u && u<='F') return 10 + (u-'A');
        return -1;
    };

    const int want = (int)(hex.size()/2);
    const int n = std::min(max_len, want);
    for (int i=0;i<n;i++){
        int hi = nib(hex[2*i+0]);
        int lo = nib(hex[2*i+1]);
        if (hi<0 || lo<0) return false;
        out[i] = (uint8_t)((hi<<4)|lo);
    }
    written = n;
    return (n>0);
}

// ==== C-ABI 구현: 선언만 있던 함수 정의를 실제로 만듭니다. ====
// 외부에서 기대하는 심볼이 'nfc_read_uid(unsigned char*, int, int)' 이므로
// 반드시 extern "C" 로 정의합니다.
extern "C" bool nfc_read_uid(uint8_t* out, int len, int timeout_s) {
    if (!out || len <= 0) return false;
    std::memset(out, 0, (size_t)len);

    sca::NfcConfig cfg{};
    cfg.poll_seconds = (timeout_s > 0 ? timeout_s : 5);

    sca::NfcResult res{};
    if (!sca::nfc_poll_once(cfg, res)) return false;
    if (!res.ok || res.uid_hex.empty()) return false;

    int written = 0;
    if (!hex_to_bytes_strict(res.uid_hex, out, len, written)) return false;

    return (written > 0);
}

// ==== 편의 오버로드: std::array 버전 ====
// (위 C-ABI 함수에 위임)
bool nfc_poll_uid(std::array<uint8_t,8>& out, int timeout_s){
    return nfc_read_uid(out.data(), (int)out.size(), timeout_s);
}

// ===== 헬퍼 =====
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

// ===== 내부 유틸 =====
void Sequencer::reset_to_idle_() {
    std::lock_guard<std::mutex> lk(m_);
    step_ = AuthStep::Idle;
    running_ = false;
    have_expected_nfc_ = false;
    have_ble_sess_ = have_ble_chal_ = false;
}

// ===== 단계 시작 =====
void Sequencer::start_sequence_() {
    if (running_) return;
    running_ = true;
    step_ = AuthStep::WaitingTCU;
    request_user_info_to_tcu_(); // TCU에게 0x102 요청
    send_auth_state_(static_cast<uint8_t>(AuthStep::WaitingTCU), AuthStateFlag::OK);
}

// 0x102 SCA_TCU_USER_INFO_REQ 송신 (플래그 1)
void Sequencer::request_user_info_to_tcu_() {
    CanFrame f{}; f.id = BCAN_ID_SCA_TCU_USER_INFO_REQ; f.dlc = 1; f.data[0] = 1;
    can_send(cfg_.can_channel.c_str(), f, 0);
}

// ===== NFC 수행 =====
bool Sequencer::perform_nfc_() {
    // TCU 기대 NFC가 먼저 와야 함
    if (!have_expected_nfc_) {
        ack_user_info_(/*index=*/1, /*state=*/1); // 누락
        send_auth_state_(static_cast<uint8_t>(AuthStep::NFC), AuthStateFlag::FAIL);
        return false;
    }
    std::array<uint8_t,8> read_uid{};
    const bool ok = nfc_poll_uid(read_uid,cfg_.nfc_timeout_s);
    if (!ok) {
        send_auth_state_(static_cast<uint8_t>(AuthStep::NFC), AuthStateFlag::FAIL);
        return false;
    }

    const bool match = (read_uid == expected_nfc_);
    send_auth_state_(static_cast<uint8_t>(AuthStep::NFC), match ? AuthStateFlag::OK : AuthStateFlag::FAIL);
    ack_user_info_(/*index=*/1, /*state=*/ match ? 0 : 1);
    return match;
}

// ===== BLE 수행 =====
bool Sequencer::perform_ble_() {
    // 세션/챌린지 4+4 바이트가 먼저 와야 함
    if (!have_ble_sess_ || !have_ble_chal_) {
        ack_user_info_(/*index=*/2, /*state=*/1); // 누락
        send_auth_state_(static_cast<uint8_t>(AuthStep::BLE), AuthStateFlag::FAIL);
        return false;
    }

    // 기대 토큰은 “세션||챌린지” 8바이트를 헥스 ASCII로 변환
    uint8_t cat[8];
    std::memcpy(cat,                 ble_sess_.data(), 4);
    std::memcpy(cat + 4,            ble_chal_.data(), 4);
    const std::string expected = to_hex_(cat, 8);

    // BLE 서비스 UUID 마지막 블록 12자리: 세션 4바이트(8 hex) + 챌린지 상위 2바이트(4 hex) 등
    // 프로젝트 규약에 맞게 고치면 됨. 임시로 세션(4B)=8hex + chal(상위2B)=4hex => 총 12hex
    const std::string last12 = to_hex_(ble_sess_.data(), 4).substr(0,8) +
                               to_hex_(ble_chal_.data(), 4).substr(0,4);

    const bool matched = sca_ble_advertise_and_wait(
        last12,                // 12-hex (UUID 마지막 블록)
        cfg_.ble_local_name,   // 로컬 이름
        cfg_.ble_timeout_s,    // 타임아웃
        expected               // 기대 ASCII(폰에서 WriteValue로 보내줄 값)
    );

    send_auth_state_(static_cast<uint8_t>(AuthStep::BLE), matched ? AuthStateFlag::OK : AuthStateFlag::FAIL);
    ack_user_info_(/*index=*/2, /*state=*/ matched ? 0 : 1);
    return matched;
}

// ===== CAN 수신 디스패치 =====
void Sequencer::on_can_rx(const CanFrame& f) {
    std::printf("[TEST] %d\n",f.id);
    switch (f.id) {
    // DCU가 인증 개시
    case BCAN_ID_DCU_SCA_USER_FACE_REQ: {
        start_sequence_();
        break;
    }
    // TCU → NFC 기대 UID (8바이트)
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
    // TCU → BLE 세션(4B)
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
    // TCU → BLE 챌린지(4B)
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

// ===== tick: 상태머신 한 스텝 =====
void Sequencer::tick() {
    AuthStep cur = step_.load();
    if (!running_) return;
bool ok;
    switch (cur) {
    case AuthStep::WaitingTCU: {
        // 기대 데이터가 다 모이면 NFC 단계로
        if (have_expected_nfc_) {
            step_ = AuthStep::NFC;
        }
        break;
    }
    case AuthStep::NFC: {
        std::printf("[NFC]\n");
        ok = perform_nfc_();
            step_ = AuthStep::NFC_Wait;
        break;
    }
    case AuthStep::NFC_Wait: {
        if (!ok) {
        std::printf("[NFC] Fail\n");
            send_auth_result_(false);
            reset_to_idle_();
        } else {
        std::printf("[NFC] End\n");
            step_ = AuthStep::BLE;
        }
        break;
    }
    case AuthStep::BLE: {
        std::printf("[BLE]\n");
        bool ok = perform_ble_();
            step_ = AuthStep::BLE_Wait;
        break;
    }
     case AuthStep::BLE_Wait: {
         
        if (!ok) {
        send_auth_result_(ok);
        reset_to_idle_();
    }
        break;
    }
    case AuthStep::Idle:
    case AuthStep::Done:
    default:
        break;
    }
}

// ===== 상태/결과 송신 =====
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
    
    std::printf("[ACK]");
    CanFrame f{}; f.id = BCAN_ID_SCA_TCU_USER_INFO_ACK; f.dlc = 2;
    f.data[0] = index; // 1:NFC, 2:BLE
    f.data[1] = state; // 0 OK, 1 누락/실패
    can_send(cfg_.can_channel.c_str(), f, 0);
}
