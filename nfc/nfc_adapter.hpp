#pragma once
#include <array>

bool nfc_poll_uid(std::array<uint8_t,8>& out, int timeout_s);
