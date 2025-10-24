#include "camera_runner.hpp"
#include "config/app_config.h"
#include <array>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

using namespace std::chrono;

static int run_and_capture(const std::string& cmd, std::string& out) {
	std::array<char, 1024> buf{};
	FILE* p = popen(cmd.c_str(), "r");
	if (!p) return 1;
	out.clear();
	while (fgets(buf.data(), buf.size(), p)) out += buf.data();
	int st = pclose(p);
	return WIFEXITED(st) ? WEXITSTATUS(st) : 1;
}

bool run_camera_app(std::string& out_user_id, int timeout_sec) {
	out_user_id.clear();
	// 카메라 앱은 stdout에 "USER=xxxx" 한 줄을 반드시 출력한다고 가정
	// 필요시 파라미터(모델 경로 등) 붙여서 바꾸면 됨.
	std::string cmd = std::string("timeout ") + std::to_string(timeout_sec) + "s " + CAMERA_BIN_PATH;

	// 간단히 한 번 실행해서 전체 stdout 파싱 (앱이 스트리밍이면 별도 설계 필요)
	std::string all;
	int rc = run_and_capture(cmd, all);
	if (all.empty()) return false;

	auto pos = all.find("USER=");
	if (pos == std::string::npos) return false;
	pos += 5;
	size_t end = all.find_first_of("\r\n", pos);
	out_user_id = all.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
	return !out_user_id.empty();
}
