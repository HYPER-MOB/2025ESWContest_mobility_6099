#include "channel.hpp"
#include <cstring>
#include <cstdlib>
#include <string>

// 내부 구조체/리스트(원본과 동일 개념)  :contentReference[oaicite:5]{index=5}
struct Job {
    int          id;
    CanFrame* frame_ref;     // 외부가 소유 (heap 권장)
    uint32_t     period_ms;
    uint64_t     next_due_ms;   // 스케줄러가 쓰는 필드(여기서는 유지만)
    Job* next;
};
struct Sub {
    int             id;
    CanFilter       filter;
    can_callback_t  cb;
    void* user;
    Sub* next;
};

// 채널 본체
struct Channel {
    std::string name;
    CanConfig   cfg{};
    Adapter* adapter{ nullptr };
    AdapterHandle h{ nullptr };

    Job* jobs{ nullptr };     int next_job_id{ 0 };
    Sub* subs{ nullptr };     int next_sub_id{ 0 };
};

// --- filter 유틸 (원본과 동일 동작) ---  :contentReference[oaicite:6]{index=6}
static bool filter_copy(CanFilter* dst, const CanFilter* src) {
    if (!dst || !src) return false;
    *dst = *src;
    if (src->type == CAN_FILTER_LIST) {
        uint32_t n = src->data.list.count;
        uint32_t* lst = src->data.list.list;
        if (n > 0 && lst) {
            auto* buf = new (std::nothrow) uint32_t[n];
            if (!buf) return false;
            std::memcpy(buf, lst, sizeof(uint32_t) * n);
            dst->data.list.list = buf;
            dst->data.list.count = n;
        }
        else {
            dst->data.list.list = nullptr;
            dst->data.list.count = 0;
        }
    }
    return true;
}
static inline bool filter_match(const CanFilter* f, uint32_t id) {
    if (!f) return true;
    switch (f->type) {
    case CAN_FILTER_RANGE: {
        uint32_t lo = f->data.range.min, hi = f->data.range.max;
        return (id >= lo && id <= hi);
    }
    case CAN_FILTER_MASK: {
        uint32_t mid = f->data.mask.id;
        uint32_t mask = f->data.mask.mask;
        return ((id & mask) == (mid & mask));
    }
    case CAN_FILTER_LIST: {
        const uint32_t* list = f->data.list.list;
        uint32_t n = f->data.list.count;
        if (!list || n == 0) return false;
        for (uint32_t i = 0; i < n; ++i) if (list[i] == id) return true;
        return false;
    }
    default: return false;
    }
}
static void filter_free(CanFilter* f) {
    if (!f) return;
    if (f->type == CAN_FILTER_LIST && f->data.list.list) {
        delete[] f->data.list.list;
        f->data.list.list = nullptr;
        f->data.list.count = 0;
    }
}

// 어댑터 → 채널 수신 분배  :contentReference[oaicite:7]{index=7}
static void on_rx_from_adapter(const CanFrame* f, void* user) {
    Channel* ch = static_cast<Channel*>(user);
    for (Sub* s = ch->subs; s; s = s->next) {
        if (!filter_match(&s->filter, f->id)) continue;
        s->cb(f, s->user);
    }
}

// --- API 구현 (원본 시그니처/동작 유지) ---  :contentReference[oaicite:8]{index=8}
can_err_t channel_start(const char* name, CanConfig cfg, Adapter* adapter, Channel** out) {
    if (!name || !out) return CAN_ERR_INVALID;

    Channel* ch = new (std::nothrow) Channel();
    if (!ch) return CAN_ERR_MEMORY;

    ch->name = name;
    ch->cfg = cfg;
    ch->adapter = adapter;

    can_err_t e = adapter->v->ch_open(adapter, name, &cfg, &ch->h);
    if (e != CAN_OK) {
        delete ch;
        return e;
    }
    if (adapter->v->ch_set_callbacks) {
        adapter->v->ch_set_callbacks(adapter, ch->h,
            on_rx_from_adapter, ch,   // on_rx
            nullptr, nullptr,         // on_err
            nullptr, nullptr          // on_bus
        );
    }
    *out = ch;
    return CAN_OK;
}

can_err_t channel_stop(Channel* ch) {
    if (!ch) return CAN_ERR_INVALID;

    // 구독 해제 및 필터 메모리 해제
    for (Sub* s = ch->subs; s; ) {
        Sub* ns = s->next;
        filter_free(&s->filter);
        delete s;
        s = ns;
    }
    ch->subs = nullptr;

    // 잡 리스트는 이 모듈이 소유(등록만) → 구조체만 정리 (frame_ref는 외부 소유)
    for (Job* j = ch->jobs; j; ) {
        Job* nj = j->next;
        delete j;
        j = nj;
    }
    ch->jobs = nullptr;

    if (ch->adapter && ch->adapter->v->ch_close) {
        ch->adapter->v->ch_close(ch->adapter, ch->h);
    }
    delete ch;
    return CAN_OK;
}

const char* channel_name(const Channel* ch) {
    return ch ? ch->name.c_str() : "";
}

can_err_t channel_write(Channel* ch, const CanFrame* frame, uint32_t timeout_ms) {
    if (!ch || !frame) return CAN_ERR_INVALID;
    return ch->adapter->v->write(ch->adapter, ch->h, frame, timeout_ms);
}

can_err_t channel_read(Channel* ch, CanFrame* out, uint32_t timeout_ms) {
    if (!ch || !out) return CAN_ERR_INVALID;
    return ch->adapter->v->read(ch->adapter, ch->h, out, timeout_ms);
}

int channel_register_job(Channel* ch, CanFrame* frame, uint32_t period_ms) {
    if (!ch || !frame || period_ms == 0) return -1;

    Job* j = new (std::nothrow) Job();
    if (!j) return -1;

    j->id = ++ch->next_job_id;
    j->frame_ref = frame;
    j->period_ms = period_ms;
    j->next_due_ms = 0;
    j->next = ch->jobs;
    ch->jobs = j;

    return j->id;
}

can_err_t channel_cancel_job(Channel* ch, int jobId) {
    if (!ch || jobId <= 0) return CAN_ERR_INVALID;

    Job** pp = &ch->jobs;
    while (*pp) {
        if ((*pp)->id == jobId) {
            Job* del = *pp;
            *pp = del->next;
            delete del;
            return CAN_OK;
        }
        pp = &(*pp)->next;
    }
    return CAN_ERR_INVALID;
}

int channel_subscribe(Channel* ch, const CanFilter* filter, can_callback_t cb, void* user) {
    if (!ch || !filter || !cb) return -1;

    Sub* s = new (std::nothrow) Sub();
    if (!s) return -1;

    if (!filter_copy(&s->filter, filter)) { delete s; return -1; }

    s->cb = cb;
    s->user = user;
    s->id = ++ch->next_sub_id;
    s->next = ch->subs;
    ch->subs = s;

    return s->id;
}

can_err_t channel_unsubscribe(Channel* ch, int subId) {
    if (!ch || subId <= 0) return CAN_ERR_INVALID;

    Sub** pp = &ch->subs;
    while (*pp) {
        if ((*pp)->id == subId) {
            Sub* del = *pp;
            *pp = del->next;
            filter_free(&del->filter);
            delete del;
            return CAN_OK;
        }
        pp = &(*pp)->next;
    }
    return CAN_ERR_INVALID;
}

can_err_t channel_recover(Channel* ch) {
    if (!ch) return CAN_ERR_INVALID;
    return ch->adapter->v->recover ? ch->adapter->v->recover(ch->adapter, ch->h) : CAN_OK;
}

can_bus_state_t channel_status(Channel* ch) {
    if (!ch) return CAN_BUS_STATE_ERROR_PASSIVE;
    return ch->adapter->v->status ? ch->adapter->v->status(ch->adapter, ch->h)
        : CAN_BUS_STATE_ERROR_ACTIVE;
}
