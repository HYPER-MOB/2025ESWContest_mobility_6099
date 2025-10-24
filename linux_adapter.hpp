#pragma once
#include "adapter.hpp"
#include <string>
#include <thread>
#include <atomic>
#include <functional>

struct LinuxPriv {
    int                 fd{-1};
    std::thread         rx_thread;
    std::atomic_bool    stop{false};

    // 콜백 (channel.cpp가 ch_set_callbacks로 내려줌)
    adapter_rx_cb_t   on_rx  {nullptr};
    void*             on_rx_user{nullptr};
    adapter_err_cb_t  on_err {nullptr};
    void*             on_err_user{nullptr};
    adapter_bus_cb_t  on_bus {nullptr};
    void*             on_bus_user{nullptr};
};

Adapter* create_linux_adapter();
