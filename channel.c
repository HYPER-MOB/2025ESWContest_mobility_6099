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

typedef struct Job {
    int          id;
    CanFrame*    frame_ref;
    uint32_t     period_ms;
    uint64_t     next_due_ms;
    struct Job*  next;
} Job;

typedef struct Sub {
    int             id;
    CanFilter       filter;
    can_callback_t  cb;
    void*           user;
    struct Sub*     next;
} Sub;

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

static void filter_free(CanFilter* f) {
    if (!f) return;
    if (f->type == CAN_FILTER_LIST && f->data.list.list) {
        free(f->data.list.list);
        f->data.list.list = NULL;
        f->data.list.count = 0;
    }
}


can_err_t       channel_start(const char* name, CanConfig cfg, Adapter* adapter, Channel** out) {
    if(!name || !out) return CAN_ERR_INVALID;

    Channel* ch = (Channel*)calloc(1, sizeof(Channel));
    if(!ch) return CAN_ERR_MEMORY;

    ch->name = strdup(name);
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

int             channel_register_job(Channel* ch, CanFrame* frame, uint32_t period_ms) {
    if (!ch || !frame || period_ms == 0) return -1;

    Job* j = (Job*)calloc(1, sizeof(Job));
    if (!j) return -1;

    j->id        = ++ch->next_job_id;
    j->frame_ref = frame; 
    j->period_ms = period_ms;
    j->next_due_ms = 0; 

    j->next = ch->jobs;
    ch->jobs = j;

    return j->id;
}

can_err_t       channel_cancel_job  (Channel* ch, int jobId) {
    if (!ch || jobId <= 0) return CAN_ERR_INVALID;

    Job** pp = &ch->jobs;
    while (*pp) {
        if ((*pp)->id == jobId) {
            Job* del = *pp;
            *pp = del->next;
            free(del);
            return CAN_OK;
        }
        pp = &(*pp)->next;
    }
    return CAN_ERR_INVALID;
}

int             channel_subscribe(Channel* ch, const CanFilter* filter, can_callback_t cb, void* user) {
    if (!ch || !filter || !cb) return -1;
    Sub* s = (Sub*)calloc(1, sizeof(Sub));
    if (!s) return -1;

    if (!filter_copy(&s->filter, filter)) { 
        free(s); 
        return -1; 
    }

    s->cb   = cb;
    s->user = user;

    s->id   = ++ch->next_sub_id;
    s->next = ch->subs;
    ch->subs = s;

    return s->id;
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