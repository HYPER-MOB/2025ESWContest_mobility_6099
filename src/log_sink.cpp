#include "log_sink.hpp"
#include <chrono>
#include <iomanip>


bool LogSink::open(const std::string& path) {
	f_.open(path, std::ios::app);
	return (bool)f_;
}


void LogSink::write(const std::string& line) {
	std::lock_guard<std::mutex> lk(mu_);
	auto now = std::chrono::system_clock::now();
	std::time_t tt = std::chrono::system_clock::to_time_t(now);
	f_ << std::put_time(std::localtime(&tt), "%F %T") << " | " << line << "
		";
		f_.flush();
}