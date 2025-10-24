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
	// ī�޶� ���� stdout�� "USER=xxxx" �� ���� �ݵ�� ����Ѵٰ� ����
	// �ʿ�� �Ķ����(�� ��� ��) �ٿ��� �ٲٸ� ��.
	std::string cmd = std::string("timeout ") + std::to_string(timeout_sec) + "s " + CAMERA_BIN_PATH;

	// ������ �� �� �����ؼ� ��ü stdout �Ľ� (���� ��Ʈ�����̸� ���� ���� �ʿ�)
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
