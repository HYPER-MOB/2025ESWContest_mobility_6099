#include "adapter.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <chrono>
#include <cstring>

// 채널 핸들(디버그용): 메모리 큐 + 콜백 보관
struct DebugChannel {
    std::string        name;
    CanConfig          cfg{};

    std::queue<CanFrame> q;
    std::mutex         m;
    std::condition_variable cv;

    // 콜백
    adapter_rx_cb_t    on_rx  = nullptr; void* on_rx_user  = nullptr;
    adapter_err_cb_t   on_err = nullptr; void* on_err_user = nullptr;
    adapter_bus_cb_t   on_bus = nullptr; void* on_bus_user = nullptr;
};

struct DebugAdapterPriv {
    // 필요시 전역 상태
};

static can_err_t dbg_probe(Adapter* self){
    (void)self;
    return CAN_OK;
}

static can_err_t dbg_open(Adapter* self, const char* name, const CanConfig* cfg, AdapterHandle* out){
    (void)self;
    if (!name || !out) return CAN_ERR_INVALID;
    auto* ch = new(std::nothrow) DebugChannel();
    if (!ch) return CAN_ERR_MEMORY;
    ch->name = name;
    if (cfg) ch->cfg = *cfg;
    *out = (AdapterHandle)ch;
    return CAN_OK;
}

static void dbg_close(Adapter* self, AdapterHandle h){
    (void)self;
    auto* ch = (DebugChannel*)h;
    if (!ch) return;
    delete ch;
}

static can_err_t dbg_set_cb(Adapter* self, AdapterHandle h,
    adapter_rx_cb_t on_rx, void* on_rx_user,
    adapter_err_cb_t on_err, void* on_err_user,
    adapter_bus_cb_t on_bus, void* on_bus_user)
{
    (void)self;
    auto* ch = (DebugChannel*)h;
    if (!ch) return CAN_ERR_INVALID;
    ch->on_rx = on_rx;       ch->on_rx_user = on_rx_user;
    ch->on_err = on_err;     ch->on_err_user = on_err_user;
    ch->on_bus = on_bus;     ch->on_bus_user = on_bus_user;
    return CAN_OK;
}

static can_err_t dbg_write(Adapter* self, AdapterHandle h, const CanFrame* fr, uint32_t /*timeout_ms*/){
    (void)self;
    auto* ch = (DebugChannel*)h;
    if (!ch || !fr) return CAN_ERR_INVALID;

    // 내부 큐에도 넣고
    {
        std::lock_guard<std::mutex> lk(ch->m);
        ch->q.push(*fr);
    }
    ch->cv.notify_one();

    // 루프백: 곧바로 on_rx 콜백 호출
    if (ch->on_rx) ch->on_rx(fr, ch->on_rx_user);
    return CAN_OK;
}

static can_err_t dbg_read(Adapter* self, AdapterHandle h, CanFrame* out, uint32_t timeout_ms){
    (void)self;
    auto* ch = (DebugChannel*)h;
    if (!ch || !out) return CAN_ERR_INVALID;

    std::unique_lock<std::mutex> lk(ch->m);
    if (timeout_ms == 0){
        if (ch->q.empty()) return CAN_ERR_AGAIN;
    } else {
        if (!ch->cv.wait_for(lk, std::chrono::milliseconds(timeout_ms), [&]{ return !ch->q.empty(); })){
            return CAN_ERR_TIMEOUT;
        }
    }
    if (ch->q.empty()) return CAN_ERR_AGAIN;
    *out = ch->q.front();
    ch->q.pop();
    return CAN_OK;
}

static can_bus_state_t dbg_status(Adapter* /*self*/, AdapterHandle /*h*/){
    return CAN_BUS_STATE_ERROR_ACTIVE;
}

static can_err_t dbg_recover(Adapter* /*self*/, AdapterHandle /*h*/){
    return CAN_OK;
}

static void dbg_destroy(Adapter* self){
    if (!self) return;
    delete (DebugAdapterPriv*)self->priv;
    self->priv = nullptr;
    delete self;
}

static AdapterVTable g_vtbl = {
    dbg_probe,
    dbg_open,
    dbg_close,
    dbg_set_cb,
    dbg_write,
    dbg_read,
    dbg_status,
    dbg_recover,
    dbg_destroy
};

// 공장 함수: can_init(CAN_DEVICE_DEBUG) 에서 호출됨
Adapter* create_adapter(can_device_t device){
    if (device != CAN_DEVICE_DEBUG){
        // 여기서 다른 device를 원한다면, 실제 socketcan 어댑터를 만들어 반환하도록 분기
        // ex) return create_linux_socketcan_adapter();
        // 지금은 디버그만 지원.
        return nullptr;
    }
    auto* a = new(std::nothrow) Adapter();
    if (!a) return nullptr;
    a->v = &g_vtbl;
    a->priv = new(std::nothrow) DebugAdapterPriv();
    return a;
}
