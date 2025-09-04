#pragma once
#include "can_api.h"

typedef struct Adapter Adapter;

typedef struct AdapterVTable {
    can_err_t (*probe)(Adapter* self);

    can_err_t (*ch_open)(Adapter* self, const char* name, const CanConfig* cfg, void** out_handle);
    void      (*ch_close)(Adapter* self, void* handle);

    can_err_t (*write)(Adapter* self, void* handle, const CanFrame* fr, uint32_t timeout_ms);
    can_err_t (*read )(Adapter* self, void* handle,       CanFrame* out, uint32_t timeout_ms);

    can_bus_state_t (*status)(Adapter* self, void* handle);
    can_err_t       (*recover)(Adapter* self, void* handle);

    // 어댑터 자체 파기
    void (*destroy)(Adapter* self);
} AdapterVTable;

struct Adapter {
    const AdapterVTable* v;
    void* priv;
 };

Adapter* create_adapter(can_device_t device);