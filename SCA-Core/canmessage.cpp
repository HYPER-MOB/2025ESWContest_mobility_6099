#include "canmessage.hpp"
#include <cstring>
#include <iostream>
// 그대로 C++로 옮김  :contentReference[oaicite:3]{index=3}
CanFrame can_encode(can_msg_id_t id, const CanMessage* msg, uint8_t dlc) {
    CanFrame fr{};                 // C++ value-init
    fr.id = id;
    fr.dlc = dlc;
    std::memcpy(fr.data, msg->raw, dlc);
    fr.flags = 0;                  // 필요시 플래그
    return fr;
}

// 그대로 C++로 옮김  :contentReference[oaicite:4]{index=4}
can_msg_id_t can_decode(const CanFrame* fr, CanMessage* out) {
    if (!fr || !out) return (can_msg_id_t)0;
    std::memset(out, 0, sizeof(*out));
    std::memcpy(out->raw, fr->data, fr->dlc);
    return (can_msg_id_t)fr->id;
}
