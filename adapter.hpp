#pragma once
#include "can_api.hpp"

// ���漱��
struct Adapter;
using AdapterHandle = void*;

// ����� �� ä�� �ݹ� (����/����/��������)
using adapter_rx_cb_t = void (*)(const CanFrame* frame, void* user);
using adapter_err_cb_t = void (*)(can_err_t err, void* user);
using adapter_bus_cb_t = void (*)(can_bus_state_t state, void* user);

// ���� ���̺� (������ ���� �������̽�)
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

// ����� ��ü
struct Adapter {
    const AdapterVTable* v;
    void* priv;
};

// ���丮(������ ����)
Adapter* create_adapter(can_device_t device);
