#include "canmessage.h"
#include <string.h>

CanFrame can_encode_pcan(can_msg_pcan_id_t id, const CanMessage* msg, uint8_t dlc){
    CanFrame fr = {0};
    fr.id   = id;
    fr.dlc  = dlc;
    memcpy(fr.data, msg->raw, dlc);
    fr.flags = 0; // 필요시 확장 ID 플래그 세팅
    return fr;
}

CanFrame can_encode_bcan(can_msg_bcan_id_t id, const CanMessage* msg, uint8_t dlc){
    CanFrame fr = {0};
    fr.id   = id;
    fr.dlc  = dlc;
    memcpy(fr.data, msg->raw, dlc);
    fr.flags = 0; // 필요시 확장 ID 플래그 세팅
    return fr;
}

can_err_t can_decode_pcan(const CanFrame* fr, can_msg_pcan_id_t* id, CanMessage* payload){
    if (!fr || !payload) return CAN_ERR_INVALID;
    memset(payload, 0, sizeof(*payload));
    memcpy(payload->raw, fr->data, fr->dlc);
    id = fr->id;
    return CAN_OK;
}

can_err_t can_decode_bcan(const CanFrame* fr, can_msg_bcan_id_t* id, CanMessage* payload){
    if (!fr || !payload) return CAN_ERR_INVALID;
    memset(payload, 0, sizeof(*payload));
    memcpy(payload->raw, fr->data, fr->dlc);
    id = fr->id;
    return CAN_OK;
}