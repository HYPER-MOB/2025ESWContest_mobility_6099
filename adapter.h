#pragma once
#include "can_api.h"

typedef struct Adapter Adapter;
typedef void* AdapterHandle;
// 어댑터 → 채널로 이벤트를 올려줄 콜백들
typedef void (*adapter_rx_cb_t)(const CanFrame* frame, void* user);
typedef void (*adapter_err_cb_t)(can_err_t err, void* user);
// (선택) 버스 상태 변화 알림
typedef void (*adapter_bus_cb_t)(can_bus_state_t state, void* user);

typedef struct AdapterVTable {
    can_err_t (*probe)(Adapter* self);

    can_err_t (*ch_open)(Adapter* self, const char* name, const CanConfig* cfg, AdapterHandle* out_handle);
    void      (*ch_close)(Adapter* self, AdapterHandle handle);

    can_err_t (*ch_set_callbacks)(
        Adapter* self, AdapterHandle h,
        adapter_rx_cb_t     on_rx,  void* on_rx_user,
        adapter_err_cb_t    on_err, void* on_err_user,
        adapter_bus_cb_t    on_bus, void* on_bus_user
    );

    can_err_t (*write)(Adapter* self, AdapterHandle handle, const CanFrame* fr, uint32_t timeout_ms);
    can_err_t (*read )(Adapter* self, AdapterHandle handle,       CanFrame* out, uint32_t timeout_ms);

    can_bus_state_t (*status)(Adapter* self, AdapterHandle handle);
    can_err_t       (*recover)(Adapter* self, AdapterHandle handle);

    int         (*ch_register_job)      (Adapter* self, int* id, AdapterHandle h, const CanFrame* fr, uint32_t period_ms);
    can_err_t   (*ch_cancel_job)        (Adapter* self, AdapterHandle h, int jobId);
    int         (*ch_register_job_ex)   (Adapter* self, int* id, AdapterHandle h, const CanFrame* initial, uint32_t period_ms, can_tx_prepare_cb_t prep, void* prep_user);

    // 어댑터 자체 파기
    void (*destroy)(Adapter* self);
} AdapterVTable;

struct Adapter {
    const AdapterVTable* v;
    void* priv;
 };

Adapter* create_adapter(can_device_t device);