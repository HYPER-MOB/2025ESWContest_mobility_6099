#pragma once
#include <atomic>
#include <string>
#include "can_ids.hpp"
#include "tcu_store.hpp"

struct SeqContext {
	std::atomic<AuthStep> step{ AuthStep::Idle };
	std::atomic<AuthFlag> flag{ AuthFlag::OK };
	std::string nfc_uid_read;
	bool ble_ok = false;
	std::string user_id;     // ī�޶� �ܰ迡�� �ĺ��� ����� ID (������ �⺻��)
};

bool run_nfc(SeqContext& ctx, const TcuProvided& tcu);
bool run_ble(SeqContext& ctx, const TcuProvided& tcu);
bool run_camera(SeqContext& ctx);
