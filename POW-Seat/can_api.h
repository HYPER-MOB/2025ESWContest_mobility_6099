#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CAN_DEVICE_NONE = 0,
    CAN_DEVICE_LINUX,
    CAN_DEVICE_ESP32,
} can_device_t;

typedef enum {
    CAN_MODE_NORMAL = 0,
    CAN_MODE_LOOPBACK,
    CAN_MODE_SILENT,
    CAN_MODE_SILENT_LOOPBACK
} can_mode_t;

typedef enum {
    CAN_BUS_STATE_ERROR_ACTIVE = 0,
    CAN_BUS_STATE_ERROR_PASSIVE,
    CAN_BUS_STATE_BUS_OFF
} can_bus_state_t;

typedef enum {
    CAN_OK = 0,
    CAN_ERR_AGAIN,
    CAN_ERR_TIMEOUT,
    CAN_ERR_INVALID,
    CAN_ERR_IO,
    CAN_ERR_BUSOFF,
    CAN_ERR_STATE,
    CAN_ERR_MEMORY,
    CAN_ERR_PERMISSION,
    CAN_ERR_NODEV
} can_err_t;

typedef enum {
    CAN_FRAME_EXTID = 1 << 0,
    CAN_FRAME_RTR = 1 << 1,
    CAN_FRAME_ERR = 1 << 2
} can_frame_flag_t;

typedef enum {
    CAN_FILTER_RANGE = 0,
    CAN_FILTER_MASK,
    CAN_FILTER_LIST
} can_filter_t;

typedef struct {
    can_filter_t type;
    union {
        struct {uint32_t min; uint32_t max;} range;
        struct {uint32_t id; uint32_t mask;} mask;
        struct {uint32_t* list; uint32_t count;} list;
    } data;
} CanFilter;

typedef struct {
    uint32_t id;      // 11-bit ID 사용 가정
    uint8_t  dlc;
    uint8_t  data[8];
    uint32_t flags;
} CanFrame;

typedef struct {
    uint8_t     channel;
    int         bitrate;
    float       samplePoint;
    int         sjw;
    can_mode_t  mode;
} CanConfig;

typedef void (*can_callback_t)(const CanFrame* frame, void* user);
typedef void (*can_tx_prepare_cb_t)(CanFrame* io_frame, void* user);

can_err_t   can_init(can_device_t device);
void        can_dispose();
can_err_t   can_open                (const char* name, CanConfig cfg);
can_err_t   can_close               (const char* name);
can_err_t   can_send                (const char* name, CanFrame frame, uint32_t timeout_ms);
can_err_t   can_recv                (const char* name, CanFrame* out, uint32_t timeout_ms);
can_err_t   can_register_job        (const char* name, int* jobId, const CanFrame* frame, uint32_t period_ms);
can_err_t   can_register_job_dynamic(const char* name, int* jobId, can_tx_prepare_cb_t prep, void* prep_user, uint32_t period_ms);
can_err_t   can_cancel_job          (const char* name, int jobId);
can_err_t   can_subscribe           (const char* name, int* subId, CanFilter filter, can_callback_t callback, void* user);
can_err_t   can_unsubscribe         (const char* name, int subId);
can_err_t   can_recover             (const char* name);
can_bus_state_t can_get_status      (const char* name);

#ifdef __cplusplus
}
#endif