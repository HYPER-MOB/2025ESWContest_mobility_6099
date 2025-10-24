#pragma once
#include <cstdint>
#include "can_api.hpp"  // �Ʒ����� ���� C++ȭ

// ���� union: payload ��  :contentReference[oaicite:0]{index=0}
union CanMessage {
    uint8_t raw[8];

    struct {
        uint8_t sig_seat_position;
        uint8_t sig_seat_angle;
        uint8_t sig_seat_front_height;
        uint8_t sig_seat_rear_height;
    } dcu_seat_order;

    struct {
        uint8_t sig_mirror_left_yaw;
        uint8_t sig_mirror_left_pitch;
        uint8_t sig_mirror_right_yaw;
        uint8_t sig_mirror_right_pitch;
        uint8_t sig_mirror_room_yaw;
        uint8_t sig_mirror_room_pitch;
    } dcu_mirror_order;

    struct {
        uint8_t sig_wheel_position;
        uint8_t sig_wheel_angle;
    } dcu_wheel_order;

    struct {
        uint8_t sig_seat_position;
        uint8_t sig_seat_angle;
        uint8_t sig_seat_front_height;
        uint8_t sig_seat_rear_height;
        uint8_t sig_seat_flag;
    } pow_seat_state;

    struct {
        uint8_t sig_mirror_left_yaw;
        uint8_t sig_mirror_left_pitch;
        uint8_t sig_mirror_right_yaw;
        uint8_t sig_mirror_right_pitch;
        uint8_t sig_mirror_room_yaw;
        uint8_t sig_mirror_room_pitch;
        uint8_t sig_seat_flag;
    } pow_mirror_state;

    struct {
        uint8_t sig_wheel_position;
        uint8_t sig_wheel_angle;
        uint8_t sig_wheel_flag;
    } pow_wheel_state;

    // ... �޽����� struct �߰�
};

// ID enum (DBC�� ���缭)  :contentReference[oaicite:1]{index=1}
typedef enum {
    BCAN_ID_DCU_SEAT_ORDER = 0x001,
    BCAN_ID_DCU_MIRROR_ORDER = 0x002,
    BCAN_ID_DCU_WHEEL_ORDER = 0x003,
    BCAN_ID_POW_SEAT_STATE = 0x100,
    BCAN_ID_POW_MIRROR_STATE = 0x200,
    BCAN_ID_POW_WHEEL_STATE = 0x300,
    // ...
} can_msg_id_t;

// ���ڵ�/���ڵ� �������̽��� �״�� ��  :contentReference[oaicite:2]{index=2}
CanFrame     can_encode(can_msg_id_t id, const CanMessage* msg, uint8_t dlc);
can_msg_id_t can_decode(const CanFrame* fr, CanMessage* out);
