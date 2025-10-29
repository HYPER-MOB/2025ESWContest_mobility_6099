
#pragma once
#include "can_api.hpp"
#include "adapter.hpp"

// opaque Ÿ�� (�����ͷθ� �ٷ�)
struct Channel;

// ä�� �����ֱ�
can_err_t       channel_start(const char* name, CanConfig cfg, Adapter* adapter, Channel** out);
can_err_t       channel_stop(Channel* ch);
const char* channel_name(const Channel* ch);

// I/O
can_err_t       channel_write(Channel* ch, const CanFrame* frame, uint32_t timeout_ms);
can_err_t       channel_read(Channel* ch, CanFrame* out, uint32_t timeout_ms);

// �ֱ� ���� �� (frame �����ʹ� �ܺΰ� ����/�������)
int             channel_register_job(Channel* ch, CanFrame* frame, uint32_t period_ms);
can_err_t       channel_cancel_job(Channel* ch, int jobId);

// ����
int             channel_subscribe(Channel* ch, const CanFilter* filter, can_callback_t cb, void* user);
can_err_t       channel_unsubscribe(Channel* ch, int subId);

// ����
can_err_t       channel_recover(Channel* ch);
can_bus_state_t channel_status(Channel* ch);
