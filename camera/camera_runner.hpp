#pragma once
#include <string>

// ī�޶� �ܺ� ���� �����ϰ� stdout���� "USER=<id>"�� ã�� out_user_id�� ����.
// ���� true/���� false. timeout_sec ���� ���(���� ����).
bool run_camera_app(std::string& out_user_id, int timeout_sec);
