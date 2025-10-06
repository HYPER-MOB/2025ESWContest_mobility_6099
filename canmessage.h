#pragma once
#include <stdint.h>
#include "can_api.h"
// 공통 union: payload 뷰
typedef union {
    uint8_t raw[8];

    struct {
        uint8_t sig_flag;
    } dcu_sca_user_face_req;

    struct {
        uint8_t sig_flag;
    } sca_tcu_user_info_req;

    struct {
        uint8_t sig_auth_step;
        uint8_t sig_auth_state;
    } sca_dcu_auth_state;

    struct {
        uint64_t sig_user_nfc;
    } tcu_sca_user_info_nfc;

    struct {
        uint64_t sig_user_ble;
    } tcu_sca_user_info_ble;

    struct {
        uint8_t sig_ack_index;
        uint8_t sig_ack_state;
    } sca_tcu_user_info_ack;
    
    struct {
        uint8_t sig_flag;
        uint8_t sig_user_id[7];
    } sca_dcu_auth_result;

    struct {
        uint8_t sig_user_id[8];
    } sca_dcu_auth_result_add;

    struct {
        uint8_t sig_flag;
    } dcu_tcu_user_profile_req;

    struct {
        uint8_t sig_seat_position;      
        uint8_t sig_seat_angle; 
        uint8_t sig_seat_front_height;  
        uint8_t sig_seat_rear_height;    
    } tcu_dcu_user_profile_seat;

    struct {
        uint8_t sig_mirror_left_yaw;  
        uint8_t sig_mirror_left_pitch;     
        uint8_t sig_mirror_right_yaw; 
        uint8_t sig_mirror_right_pitch;    
        uint8_t sig_mirror_room_yaw;    
        uint8_t sig_mirror_room_pitch; 
    } tcu_dcu_user_profile_mirror;
    
    struct {
        uint8_t sig_wheel_position;      
        uint8_t sig_wheel_angle;  
    } tcu_dcu_user_profile_wheel;

    struct {
        uint8_t sig_ack_index;
        uint8_t sig_ack_state;
    } dcu_tcu_user_profile_ack;

    struct {
        uint8_t sig_seat_position;      
        uint8_t sig_seat_angle; 
        uint8_t sig_seat_front_height;  
        uint8_t sig_seat_rear_height;    
    } dcu_tcu_user_profile_seat_update;

    struct {
        uint8_t sig_mirror_left_yaw;  
        uint8_t sig_mirror_left_pitch;     
        uint8_t sig_mirror_right_yaw; 
        uint8_t sig_mirror_right_pitch;    
        uint8_t sig_mirror_room_yaw;    
        uint8_t sig_mirror_room_pitch; 
    } dcu_tcu_user_profile_mirror_update;
    
    struct {
        uint8_t sig_wheel_position;      
        uint8_t sig_wheel_angle;  
    } dcu_tcu_user_profile_wheel_update;

    struct {
        uint8_t sig_ack_index;
        uint8_t sig_ack_state;
    } tcu_dcu_user_profile_update_ack;

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

    // ... 메시지별 struct 추가
} CanMessage;

// ID enum (DBC에 맞춰서)
typedef enum {
    PCAN_ID_DCU_SCA_USER_FACE_REQ               = 0x001,
    PCAN_ID_SCA_TCU_USER_INFO_REQ               = 0x002,
    PCAN_ID_SCA_DCU_AUTH_STATE                  = 0x003,
    PCAN_ID_TCU_SCA_USER_INFO                   = 0x004,
    
    PCAN_ID_TCU_SCA_USER_INFO_NFC               = 0x007,
    PCAN_ID_TCU_SCA_USER_INFO_BLE               = 0x008,
    PCAN_ID_SCA_TCU_USER_INFO_ACK               = 0x014,
    PCAN_ID_SCA_DCU_AUTH_RESULT                 = 0x005,
    PCAN_ID_SCA_DCU_AUTH_RESULT_ADD             = 0x006,
    PCAN_ID_DCU_TCU_USER_PROFILE_REQ            = 0x011,
    PCAN_ID_TCU_DCU_USER_PROFILE_SEAT           = 0x051,
    PCAN_ID_TCU_DCU_USER_PROFILE_MIRROR         = 0x052,
    PCAN_ID_TCU_DCU_USER_PROFILE_WHEEL          = 0x053,
    PCAN_ID_DCU_TCU_USER_PROFILE_ACK            = 0x055,
    PCAN_ID_DCU_TCU_USER_PROFILE_SEAT_UPDATE    = 0x061,
    PCAN_ID_DCU_TCU_USER_PROFILE_MIRROR_UPDATE  = 0x062,
    PCAN_ID_DCU_TCU_USER_PROFILE_WHEEL_UPDATE   = 0x063,
    PCAN_ID_TCU_DCU_USER_PROFILE_UPDATE_ACK     = 0x065
    // ...
} can_msg_pcan_id_t;

typedef enum {
    BCAN_ID_DCU_SEAT_ORDER                  = 0x001,
    BCAN_ID_DCU_MIRROR_ORDER                = 0x002,
    BCAN_ID_DCU_WHEEL_ORDER                 = 0x003,
    BCAN_ID_POW_SEAT_STATE                  = 0x100,
    BCAN_ID_POW_MIRROR_STATE                = 0x200,
    BCAN_ID_POW_WHEEL_STATE                 = 0x300
    // ...
} can_msg_bcan_id_t;


CanFrame        can_encode_pcan(can_msg_pcan_id_t id, const CanMessage* msg, uint8_t dlc);
CanFrame        can_encode_bcan(can_msg_bcan_id_t id, const CanMessage* msg, uint8_t dlc);
can_err_t       can_decode_pcan(const CanFrame* fr, can_msg_pcan_id_t* id, CanMessage* payload);
can_err_t       can_decode_bcan(const CanFrame* fr, can_msg_bcan_id_t* id, CanMessage* payload);