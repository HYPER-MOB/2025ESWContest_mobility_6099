#pragma once
#include <string>

bool sca_ble_advertise_and_wait(const std::string& last12,
                                const std::string& name,
                                int timeout_s,
                                const std::string& expected_ascii);
