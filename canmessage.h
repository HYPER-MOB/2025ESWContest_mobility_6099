#pragma once
#include <stdint.h>
#include "can_api.h"
// 공통 union: payload 뷰
typedef union {
    uint8_t raw[8];
    struct {
        uint8_t sig_flag;
    } dcu_reset;

    struct {
        uint8_t sig_index;
        uint8_t sig_status;
    } dcu_reset_ack;

    struct {
        uint8_t sig_flag;
    } sca_dcu_driver_event;

    struct {
        uint8_t sig_flag;
    } dcu_sca_drive_status;

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
        uint32_t sig_user_info_index;
        float    sig_user_info_value;
    } tcu_sca_user_info;

    struct {
        uint64_t sig_user_nfc;
    } tcu_sca_user_info_nfc;

    struct {
        uint64_t sig_data;
    } tcu_sca_user_info_ble_sess;

    struct {
        uint64_t sig_data;
    } tcu_sca_user_info_ble_chall;

    struct {
        uint64_t sig_data;
    } tcu_sca_user_info_ble_flag;

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
        uint8_t sig_seat_is_seated;
        uint8_t sig_seat_status;       
    } pow_seat_state;

    struct {
        uint8_t sig_mirror_left_yaw;  
        uint8_t sig_mirror_left_pitch;     
        uint8_t sig_mirror_right_yaw; 
        uint8_t sig_mirror_right_pitch;    
        uint8_t sig_mirror_room_yaw;    
        uint8_t sig_mirror_room_pitch;   
        uint8_t sig_mirror_status;       
    } pow_mirror_state;

    struct {
        uint8_t sig_wheel_position;      
        uint8_t sig_wheel_angle;  
        uint8_t sig_wheel_status;  
    } pow_wheel_state;

    struct {
        uint8_t sig_seat_position_button;      
        uint8_t sig_seat_angle_button; 
        uint8_t sig_seat_front_height_button;  
        uint8_t sig_seat_rear_height_button;    
    } dcu_seat_button;

    struct {
        uint8_t sig_mirror_left_yaw_button;  
        uint8_t sig_mirror_left_pitch_button;     
        uint8_t sig_mirror_right_yaw_button; 
        uint8_t sig_mirror_right_pitch_button;    
        uint8_t sig_mirror_room_yaw_button;    
        uint8_t sig_mirror_room_pitch_button; 
    } dcu_mirror_button;
    
    struct {
        uint8_t sig_wheel_position_button;      
        uint8_t sig_wheel_angle_button;  
    } dcu_wheel_button;

    // ... 메시지별 struct 추가
} CanMessage;

// ID enum (DBC에 맞춰서)
typedef enum {
    PCAN_ID_DCU_RESET                           = 0x001,
    PCAN_ID_DCU_RESET_ACK                       = 0x002,

    PCAN_ID_SCA_DCU_DRIVER_EVENT                = 0x003,
    PCAN_ID_DCU_SCA_DRIVE_STATUS                = 0x005,

    PCAN_ID_DCU_SCA_USER_FACE_REQ               = 0x101,
    PCAN_ID_SCA_TCU_USER_INFO_REQ               = 0x102,
    PCAN_ID_SCA_DCU_AUTH_STATE                  = 0x103,
    PCAN_ID_TCU_SCA_USER_INFO                   = 0x104,
    PCAN_ID_TCU_SCA_USER_INFO_NFC               = 0x107,
    PCAN_ID_TCU_SCA_USER_INFO_BLE_SESS          = 0x108,
    PCAN_ID_TCU_SCA_USER_INFO_BLE_CHALL         = 0x109,
    PCAN_ID_TCU_SCA_USER_INFO_BLE_FLAG          = 0x110,

    PCAN_ID_SCA_TCU_USER_INFO_ACK               = 0x111,
    PCAN_ID_SCA_DCU_AUTH_RESULT                 = 0x112,
    PCAN_ID_SCA_DCU_AUTH_RESULT_ADD             = 0x113,

    PCAN_ID_DCU_TCU_USER_PROFILE_REQ            = 0x201,
    PCAN_ID_TCU_DCU_USER_PROFILE_SEAT           = 0x202,
    PCAN_ID_TCU_DCU_USER_PROFILE_MIRROR         = 0x203,
    PCAN_ID_TCU_DCU_USER_PROFILE_WHEEL          = 0x204,
    PCAN_ID_DCU_TCU_USER_PROFILE_ACK            = 0x205,

    PCAN_ID_DCU_TCU_USER_PROFILE_SEAT_UPDATE    = 0x206,
    PCAN_ID_DCU_TCU_USER_PROFILE_MIRROR_UPDATE  = 0x207,
    PCAN_ID_DCU_TCU_USER_PROFILE_WHEEL_UPDATE   = 0x208,
    PCAN_ID_TCU_DCU_USER_PROFILE_UPDATE_ACK     = 0x209,
    // ...
} can_msg_pcan_id_t;

typedef enum {
    PCAN_DLC_DCU_RESET                           = 1,
    PCAN_DLC_DCU_RESET_ACK                       = 2,

    PCAN_DLC_SCA_DCU_DRIVER_EVENT                = 1,
    PCAN_DLC_DCU_SCA_DRIVE_STATUS                = 1,

    PCAN_DLC_DCU_SCA_USER_FACE_REQ               = 1,
    PCAN_DLC_SCA_TCU_USER_INFO_REQ               = 1,
    PCAN_DLC_SCA_DCU_AUTH_STATE                  = 2,
    PCAN_DLC_TCU_SCA_USER_INFO                   = 8,
    PCAN_DLC_TCU_SCA_USER_INFO_NFC               = 8,
    PCAN_DLC_TCU_SCA_USER_INFO_BLE_SESS          = 8,
    PCAN_DLC_TCU_SCA_USER_INFO_BLE_CHALL         = 8,
    PCAN_DLC_TCU_SCA_USER_INFO_BLE_FLAG          = 8,

    PCAN_DLC_SCA_TCU_USER_INFO_ACK               = 2,
    PCAN_DLC_SCA_DCU_AUTH_RESULT                 = 8,
    PCAN_DLC_SCA_DCU_AUTH_RESULT_ADD             = 8,

    PCAN_DLC_DCU_TCU_USER_PROFILE_REQ            = 1,
    PCAN_DLC_TCU_DCU_USER_PROFILE_SEAT           = 4,
    PCAN_DLC_TCU_DCU_USER_PROFILE_MIRROR         = 6,
    PCAN_DLC_TCU_DCU_USER_PROFILE_WHEEL          = 2,
    PCAN_DLC_DCU_TCU_USER_PROFILE_ACK            = 2,

    PCAN_DLC_DCU_TCU_USER_PROFILE_SEAT_UPDATE    = 4,
    PCAN_DLC_DCU_TCU_USER_PROFILE_MIRROR_UPDATE  = 6,
    PCAN_DLC_DCU_TCU_USER_PROFILE_WHEEL_UPDATE   = 2,
    PCAN_DLC_TCU_DCU_USER_PROFILE_UPDATE_ACK     = 2,
    // ...
} can_msg_pcan_dlc_t;

typedef enum {
    BCAN_ID_DCU_RESET                       = 0x001,
    BCAN_ID_DCU_RESET_ACK                   = 0x002,

    BCAN_ID_DCU_SEAT_ORDER                  = 0x101,
    BCAN_ID_DCU_MIRROR_ORDER                = 0x102,
    BCAN_ID_DCU_WHEEL_ORDER                 = 0x103,

    BCAN_ID_POW_SEAT_STATE                  = 0x201,
    BCAN_ID_POW_MIRROR_STATE                = 0x202,
    BCAN_ID_POW_WHEEL_STATE                 = 0x203,

    BCAN_ID_DCU_SEAT_BUTTON                 = 0x301,
    BCAN_ID_DCU_MIRROR_BUTTON               = 0x302,
    BCAN_ID_DCU_WHEEL_BUTTON                = 0x303,
    // ...
} can_msg_bcan_id_t;

typedef enum {
    BCAN_DLC_DCU_RESET                       = 1,
    BCAN_DLC_DCU_RESET_ACK                   = 2,
    BCAN_DLC_DCU_SEAT_ORDER                  = 4,
    BCAN_DLC_DCU_MIRROR_ORDER                = 6,
    BCAN_DLC_DCU_WHEEL_ORDER                 = 2,
    BCAN_DLC_POW_SEAT_STATE                  = 6,
    BCAN_DLC_POW_MIRROR_STATE                = 7,
    BCAN_DLC_POW_WHEEL_STATE                 = 3,
    BCAN_DLC_DCU_SEAT_BUTTON                 = 4,
    BCAN_DLC_DCU_MIRROR_BUTTON               = 6,
    BCAN_DLC_DCU_WHEEL_BUTTON                = 2,
} can_msg_bcan_dlc_t;


CanFrame        can_encode_pcan(can_msg_pcan_id_t id, const CanMessage* msg, uint8_t dlc);
CanFrame        can_encode_bcan(can_msg_bcan_id_t id, const CanMessage* msg, uint8_t dlc);
can_err_t       can_decode_pcan(const CanFrame* fr, can_msg_pcan_id_t* id, CanMessage* payload);
can_err_t       can_decode_bcan(const CanFrame* fr, can_msg_bcan_id_t* id, CanMessage* payload);