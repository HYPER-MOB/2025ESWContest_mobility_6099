#pragma once
#include "can_api.h"
#include "adapter.h"

typedef struct Channel Channel;

can_err_t       channel_start(const char* name, CanConfig cfg, Adapter* adapter, Channel** out);
can_err_t       channel_stop(Channel* ch); 
can_err_t       channel_write(Channel* ch, const CanFrame* frame, uint32_t timeout_ms);
can_err_t       channel_read(Channel* ch, CanFrame* out, uint32_t timeout_ms);
int             channel_register_job(Channel* ch, const CanFrame* frame, uint32_t period_ms);
int             channel_register_job_ex(Channel* ch, const CanFrame* initial, uint32_t period_ms, can_tx_prepare_cb_t prep, void* prep_user);
can_err_t       channel_cancel_job  (Channel* ch, int jobId);
int             channel_subscribe(Channel* ch, const CanFilter* filter, can_callback_t cb, void* user);
can_err_t       channel_unsubscribe(Channel* ch, int subId);
const char*     channel_name(const Channel* ch);
can_err_t       channel_recover(Channel* ch);
can_bus_state_t channel_status(Channel* ch);
