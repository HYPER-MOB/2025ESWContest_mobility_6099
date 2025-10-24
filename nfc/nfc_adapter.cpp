#include "nfc_adapter.hpp"
#include <cstdint>
#include <array>

// ✅ 이미 네 프로젝트에 있는 NFC 함수로 “그대로 포워딩”.
//    아래 extern 선언은 네 nfc_reader가 내보내는 실제 함수 이름/시그니처에 맞춰 수정해도 됨.
extern bool nfc_read_uid(uint8_t* out, int len, int timeout_s);

bool nfc_poll_uid(std::array<uint8_t,8>& out, int timeout_s) {
    return nfc_read_uid(out.data(), 8, timeout_s);
}
