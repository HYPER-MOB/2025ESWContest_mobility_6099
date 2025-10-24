#pragma once
#include <string>

// 카메라 외부 앱을 실행하고 stdout에서 "USER=<id>"를 찾아 out_user_id에 저장.
// 성공 true/실패 false. timeout_sec 동안 대기(간단 폴링).
bool run_camera_app(std::string& out_user_id, int timeout_sec);
