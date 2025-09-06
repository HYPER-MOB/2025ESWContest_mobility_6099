#include "can_api.h"
#include "channel.h"
#include "adapter.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct ChannelNode {
    Channel* ch;
    struct ChannelNode* next;
} ChannelNode;

typedef struct {
    bool            initialized;
    Adapter*        adapter;
    ChannelNode*    head;
} can_api_state_t;

static can_api_state_t g_state = { false, CAN_DEVICE_NONE, NULL };

static Channel* find_by_name(const char* name) {
    for(ChannelNode* n = g_state.head; n; n = n->next) {
       if(strcmp(channel_name(n->ch), name) == 0) return n->ch;
    }
    return NULL;
}

can_err_t   can_init(can_device_t device) {
    if (g_state.initialized) return CAN_ERR_STATE;
    Adapter* ad = create_adapter(device);
    if(!ad) return CAN_ERR_NODEV;

    can_err_t e = ad->v->probe(ad);
    if(e != CAN_OK) {
        ad->v->destroy(ad);
        return e;
    }

    memset(&g_state, 0, sizeof(g_state));
    g_state.initialized = true;
    g_state.adapter = ad;
    return CAN_OK;
}

can_err_t   can_open(const char* name, CanConfig cfg) {
    if(!g_state.initialized)        return CAN_ERR_STATE;
    if(!name || name[0] == '\0')    return CAN_ERR_INVALID;
    if(find_by_name(name))          return CAN_ERR_INVALID;

    Channel* ch = NULL;
    can_err_t e = channel_start(name, cfg, g_state.adapter, &ch);
    if (e != CAN_OK) return e;

    ChannelNode* node = (ChannelNode*)calloc(1, sizeof(ChannelNode));
    if(!node) {
        channel_stop(ch);
        return CAN_ERR_MEMORY;
    }
    node->ch = ch;
    node->next = g_state.head;
    g_state.head = node;

    return CAN_OK;
}

can_err_t   can_close(const char* name) {
    if (!g_state.initialized) return CAN_ERR_STATE;
    if (!name) return CAN_ERR_INVALID;

    ChannelNode** pp = &g_state.head;
    while (*pp) {
        if (strcmp(channel_name((*pp)->ch), name) == 0) {
            ChannelNode* del = *pp;
            *pp = del->next;
            channel_stop(del->ch);
            free(del);
            return CAN_OK;
        }
        pp = &(*pp)->next;
    }
    return CAN_ERR_INVALID;
}

void        can_dispose(void)
{
    if(!g_state.initialized) return;

    ChannelNode* node = g_state.head;
    while(node) {
        ChannelNode* next = node->next;
        channel_stop(node->ch);
        free(node);
        node = next;
    }
    g_state.head = NULL;
    if(g_state.adapter && g_state.adapter->v->destroy) {
        g_state.adapter->v->destroy(g_state.adapter);
    }
    g_state.adapter = NULL;
    g_state.initialized = false;
}

can_err_t   can_send(const char* name, CanFrame frame, uint32_t timeout_ms) {
    if(!g_state.initialized) return CAN_ERR_STATE;
    if(!name || name[0] == '\0') return CAN_ERR_INVALID;

    Channel* ch = find_by_name(name);
    if(!ch) return CAN_ERR_INVALID;

    return channel_write(ch, &frame, timeout_ms);
}

can_err_t   can_recv(const char* name, CanFrame* out, uint32_t timeout_ms) {
    if(!g_state.initialized) return CAN_ERR_STATE;
    if(!name || name[0] == '\0') return CAN_ERR_INVALID;

    Channel* ch = find_by_name(name);
    if(!ch) return CAN_ERR_INVALID;

    return channel_read(ch, out, timeout_ms);  
}

int         can_register_job(const char* name, CanFrame* frame, uint32_t period_ms) {
    if(!g_state.initialized) return CAN_ERR_STATE;
    if(!name || name[0] == '\0') return CAN_ERR_INVALID;

    Channel* ch = find_by_name(name);
    if(!ch) return CAN_ERR_INVALID;

    return channel_register_job(ch, frame, period_ms);
}

can_err_t   can_cancel_job(const char* name, int jobId) {
    if(!g_state.initialized) return CAN_ERR_STATE;
    if(!name || name[0] == '\0') return CAN_ERR_INVALID;

    Channel* ch = find_by_name(name);
    if(!ch) return CAN_ERR_INVALID;

    return channel_cancel_job(ch, jobId);
}

int         can_subscribe(const char* name, CanFilter filter, can_callback_t callback, void* user) {
    if(!g_state.initialized) return CAN_ERR_STATE;
    if(!name || name[0] == '\0') return CAN_ERR_INVALID;

    Channel* ch = find_by_name(name);
    if(!ch) return CAN_ERR_INVALID;

    return channel_subscribe(ch, &filter, callback, user);
}

can_err_t   can_unsubscribe(const char* name, int subId) {
    if(!g_state.initialized) return CAN_ERR_STATE;
    if(!name || name[0] == '\0') return CAN_ERR_INVALID;

    Channel* ch = find_by_name(name);
    if(!ch) return CAN_ERR_INVALID;

    return channel_unsubscribe(ch, subId);
}

can_err_t   can_recover(const char* name) {
    Channel* ch = find_by_name(name);
    return ch ? channel_recover(ch) : CAN_ERR_INVALID;
}

can_bus_state_t can_get_status(const char* name) {
    Channel* ch = find_by_name(name);
    return ch ? channel_status(ch) : CAN_BUS_STATE_ERROR_PASSIVE;
}