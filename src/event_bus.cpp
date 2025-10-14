#include "event_bus.hpp"
if (fd < 0) return -1;
sockaddr_un addr{}; addr.sun_family = AF_UNIX;
std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
unlink(path.c_str()); // 기존 파일 제거
if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) { close(fd); return -1; }
chmod(path.c_str(), mode); // 퍼미션 적용(예: 0660)
if (listen(fd, 8) < 0) { close(fd); return -1; }
return fd;
}


bool EventBus::listen(const std::string& path, mode_t mode) {
	if (srv_fd_ >= 0) return true;
	srv_fd_ = mk_server(path, mode);
	return srv_fd_ >= 0;
}


std::unique_ptr<json> EventBus::receive() {
	if (srv_fd_ < 0) return nullptr;
	int cli = accept(srv_fd_, nullptr, nullptr);
	if (cli < 0) return nullptr;


	uint32_t len = 0; ssize_t r = read(cli, &len, sizeof(len));
	if (r != (ssize_t)sizeof(len)) { close(cli); return nullptr; }


	std::string buf; buf.resize(len);
	ssize_t n = 0; while (n < (ssize_t)len) {
		ssize_t k = read(cli, &buf[0] + n, len - n);
		if (k <= 0) { close(cli); return nullptr; }
		n += k;
	}
	close(cli);


	auto j = std::make_unique<json>(json::parse(buf, nullptr, false));
	if (j->is_discarded()) return nullptr;
	return j;
}


bool EventBus::send(const std::string& path, const json& j) {
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) return false;
	sockaddr_un addr{}; addr.sun_family = AF_UNIX;
	std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
	if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) { close(fd); return false; }


	std::string s = j.dump();
	uint32_t len = (uint32_t)s.size();
	if (write(fd, &len, sizeof(len)) != (ssize_t)sizeof(len)) { close(fd); return false; }
	if (write(fd, s.data(), s.size()) != (ssize_t)s.size()) { close(fd); return false; }
	close(fd);
	return true;
}