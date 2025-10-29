
#pragma once
#include "can_api.hpp"
#include "adapter.hpp"

// opaque 타입 (포인터로만 다룸)
struct Channel;

// 채널 수명주기
can_err_t       channel_start(const char* name, CanConfig cfg, Adapter* adapter, Channel** out);
can_err_t       channel_stop(Channel* ch);
const char* channel_name(const Channel* ch);

// I/O
can_err_t       channel_write(Channel* ch, const CanFrame* frame, uint32_t timeout_ms);
can_err_t       channel_read(Channel* ch, CanFrame* out, uint32_t timeout_ms);

// 주기 전송 잡 (frame 포인터는 외부가 소유/수명관리)
int             channel_register_job(Channel* ch, CanFrame* frame, uint32_t period_ms);
can_err_t       channel_cancel_job(Channel* ch, int jobId);

// 구독
int             channel_subscribe(Channel* ch, const CanFilter* filter, can_callback_t cb, void* user);
can_err_t       channel_unsubscribe(Channel* ch, int subId);

// 상태
can_err_t       channel_recover(Channel* ch);
can_bus_state_t channel_status(Channel* ch);
