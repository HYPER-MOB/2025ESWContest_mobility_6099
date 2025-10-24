#pragma once
#include "can_api.hpp"

// 전방선언
struct Adapter;
using AdapterHandle = void*;

// 어댑터 → 채널 콜백 (수신/에러/버스상태)
using adapter_rx_cb_t = void (*)(const CanFrame* frame, void* user);
using adapter_err_cb_t = void (*)(can_err_t err, void* user);
using adapter_bus_cb_t = void (*)(can_bus_state_t state, void* user);

// 가상 테이블 (원본과 동일 인터페이스)
struct AdapterVTable {
    can_err_t(*probe)(Adapter* self);

    can_err_t(*ch_open)(Adapter* self, const char* name, const CanConfig* cfg, AdapterHandle* out_handle);
    void      (*ch_close)(Adapter* self, AdapterHandle handle);

    can_err_t(*ch_set_callbacks)(
        Adapter* self, AdapterHandle h,
        adapter_rx_cb_t     on_rx, void* on_rx_user,
        adapter_err_cb_t    on_err, void* on_err_user,
        adapter_bus_cb_t    on_bus, void* on_bus_user
        );

    can_err_t(*write)(Adapter* self, AdapterHandle handle, const CanFrame* fr, uint32_t timeout_ms);
    can_err_t(*read)(Adapter* self, AdapterHandle handle, CanFrame* out, uint32_t timeout_ms);

    can_bus_state_t(*status)(Adapter* self, AdapterHandle handle);
    can_err_t(*recover)(Adapter* self, AdapterHandle handle);

    void (*destroy)(Adapter* self);
};

// 어댑터 본체
struct Adapter {
    const AdapterVTable* v;
    void* priv;
};

// 팩토리(원본과 동일)
Adapter* create_adapter(can_device_t device);
