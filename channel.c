#include "channel.h"
#include <stdlib.h>
#include <string.h>

struct Channel {
    char*       name;
    CanConfig   cfg;
    Adapter*    adapter;
    void*       h;

    struct Job* jobs;
    int         next_job_id;

    struct Sub* subs;
    int         next_sub_id;
};

typedef struct Sub {
    int             id;
    CanFilter       filter;
    can_callback_t  cb;
    void*           user;
    struct Sub*     next;
} Sub;

static char* xstrdup(const char* s){
    if(!s) return NULL;
    size_t n = strlen(s) + 1;
    char* p = (char*)malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

static int filter_copy(CanFilter* dst, const CanFilter* src) {
    if (!dst || !src) return 0;
    *dst = *src;
    // type이 list인 필터는 내부 배열도 복사해 주어야 한다.
    if (src->type == CAN_FILTER_LIST) {
        uint32_t n = src->data.list.count;
        uint32_t* lst = src->data.list.list;
        if (n > 0 && lst) {
            uint32_t* buf = (uint32_t*)malloc(sizeof(uint32_t) * n);
            if (!buf) return 0;
            memcpy(buf, lst, sizeof(uint32_t) * n);
            dst->data.list.list = buf;
            dst->data.list.count = n;
        } else {
            dst->data.list.list = NULL;
            dst->data.list.count = 0;
        }
    }
    return 1;
}

static inline int filter_match(const CanFilter* f, uint32_t id) {
    if (!f) return 1; // 필터 없으면 통과
    switch (f->type) {
    case CAN_FILTER_RANGE: {
        uint32_t lo = f->data.range.min;
        uint32_t hi = f->data.range.max;
        return (id >= lo && id <= hi);
    }
    case CAN_FILTER_MASK: {
        uint32_t mid  = f->data.mask.id;
        uint32_t mask = f->data.mask.mask;
        return ((id & mask) == (mid & mask));
    }
    case CAN_FILTER_LIST: {
        const uint32_t* list = f->data.list.list;
        uint32_t n = f->data.list.count;
        if (!list || n == 0) return 0;
        for (uint32_t i = 0; i < n; ++i) {
            if (list[i] == id) return 1;
        }
        return 0;
    }
    default:
        return 0;
    }
}

static void filter_free(CanFilter* f) {
    if (!f) return;
    if (f->type == CAN_FILTER_LIST && f->data.list.list) {
        free(f->data.list.list);
        f->data.list.list = NULL;
        f->data.list.count = 0;
    }
}

static void on_rx_from_adapter(const CanFrame* f, void* user) {
    Channel* ch = (Channel*)user;
    for (Sub* s = ch->subs; s; s = s->next) {
        if (!filter_match(&s->filter, f->id)) continue;
        can_callback_t cb = s->cb;
        void* u = s->user;
        cb(f, u);
    }
}

can_err_t       channel_start(const char* name, CanConfig cfg, Adapter* adapter, Channel** out) {
    if(!name || !out) return CAN_ERR_INVALID;

    Channel* ch = (Channel*)calloc(1, sizeof(Channel));
    if(!ch) return CAN_ERR_MEMORY;

    ch->name = xstrdup(name);
    if(!ch->name) {
        free(ch);
        return CAN_ERR_MEMORY;
    }
    ch->cfg = cfg;
    ch->adapter = adapter;

    can_err_t e = adapter->v->ch_open(adapter, name, &cfg, &ch->h);
    if(e != CAN_OK) {
        free(ch->name);
        free(ch);
        return e;
    }
    if (adapter->v->ch_set_callbacks) {
        adapter->v->ch_set_callbacks(adapter, ch->h,
            on_rx_from_adapter, ch,   // on_rx
            NULL, NULL,                // on_err
            NULL, NULL                 // on_bus
        );
    }
    *out = ch;
    return CAN_OK;
}

can_err_t       channel_stop(Channel* ch) {
    if(!ch) return CAN_ERR_INVALID;

    Sub* s = ch->subs;
    while (s) {
        Sub* ns = s->next;
        filter_free(&s->filter);
        free(s);
        s = ns;
    }

    if(ch->adapter && ch->adapter->v->ch_close) {
        ch->adapter->v->ch_close(ch->adapter, ch->h);
    }
    free(ch->name);
    free(ch);
    return CAN_OK;
}

const char*     channel_name(const Channel* ch) {
    return ch ? ch->name : "";
}

can_err_t       channel_write(Channel* ch, const CanFrame* frame, uint32_t timeout_ms) {
    if(!ch || !frame) return CAN_ERR_INVALID;
    return ch->adapter->v->write(ch->adapter, ch->h, frame, timeout_ms);
}

can_err_t       channel_read(Channel* ch, CanFrame* out, uint32_t timeout_ms) {
    if(!ch || !out) return CAN_ERR_INVALID;
    return ch->adapter->v->read(ch->adapter, ch->h, out, timeout_ms);
}

can_err_t       channel_register_job(Channel* ch, int* jobId, const CanFrame* frame, uint32_t period_ms){
    if (!ch || !frame || period_ms==0) return CAN_ERR_INVALID;
    if (!ch->adapter || !ch->adapter->v->ch_register_job) return CAN_ERR_INVALID;
    return ch->adapter->v->ch_register_job(ch->adapter, jobId, ch->h, frame, period_ms);
}

can_err_t       channel_register_job_ex(Channel* ch, int* jobId, const CanFrame* initial, uint32_t period_ms, can_tx_prepare_cb_t prep, void* prep_user) {
    if (!ch || !initial || period_ms==0) return CAN_ERR_INVALID;
    if (!ch->adapter || !ch->adapter->v->ch_register_job_ex) return CAN_ERR_INVALID;
    return ch->adapter->v->ch_register_job_ex(ch->adapter, jobId, ch->h, initial, period_ms, prep, prep_user);

}

can_err_t       channel_cancel_job(Channel* ch, int jobId) {
    if (!ch || jobId<=0) return CAN_ERR_INVALID;
    if (!ch->adapter || !ch->adapter->v->ch_cancel_job) return CAN_ERR_INVALID;
    return ch->adapter->v->ch_cancel_job(ch->adapter, ch->h, jobId);
}

can_err_t       channel_subscribe(Channel* ch, int* subId, const CanFilter* filter, can_callback_t cb, void* user) {
    if (!ch || !filter || !cb) return CAN_ERR_INVALID;
    Sub* s = (Sub*)calloc(1, sizeof(Sub));
    if (!s) return CAN_ERR_INVALID;

    if (!filter_copy(&s->filter, filter)) { 
        free(s); 
        return CAN_ERR_INVALID; 
    }

    s->cb   = cb;
    s->user = user;

    s->id   = ++ch->next_sub_id;
    s->next = ch->subs;
    ch->subs = s;

    subId = s->id;

    return CAN_OK;
}

can_err_t       channel_unsubscribe(Channel* ch, int subId) {
    if (!ch || subId <= 0) return CAN_ERR_INVALID;

    Sub** pp = &ch->subs;
    while (*pp) {
        if ((*pp)->id == subId) {
            Sub* del = *pp;
            *pp = del->next;
            filter_free(&del->filter);
            free(del);
            return CAN_OK;
        }
        pp = &(*pp)->next;
    }
    return CAN_ERR_INVALID;
}

can_err_t       channel_recover(Channel* ch) {
    if (!ch) return CAN_ERR_INVALID;
    return ch->adapter->v->recover ? 
        ch->adapter->v->recover(ch->adapter, ch->h) : CAN_OK;
}

can_bus_state_t channel_status(Channel* ch) {
    if (!ch) return CAN_BUS_STATE_ERROR_PASSIVE;
    return ch->adapter->v->status ? 
        ch->adapter->v->status(ch->adapter, ch->h) : CAN_BUS_STATE_ERROR_ACTIVE;
}