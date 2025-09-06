#include "canmessage.h"
#include <string.h>

CanFrame can_encode(can_msg_id_t id, const CanMessage* msg, uint8_t dlc){
    CanFrame fr = {0};
    fr.id   = id;
    fr.dlc  = dlc;
    memcpy(fr.data, msg->raw, dlc);
    fr.flags = 0; // 필요시 확장 ID 플래그 세팅
    return fr;
}

can_msg_id_t can_decode(const CanFrame* fr, CanMessage* out){
    if (!fr || !out) return 0;
    memset(out, 0, sizeof(*out));
    memcpy(out->raw, fr->data, fr->dlc);
    return (can_msg_id_t)fr->id;
}