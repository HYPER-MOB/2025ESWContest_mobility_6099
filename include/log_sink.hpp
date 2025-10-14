#pragma once
#include <string>
#include <fstream>
#include <mutex>


class LogSink {
public:
	bool open(const std::string& path); ///< 파일 오픈(append)
	void write(const std::string& line);///< 타임스탬프 + 라인 기록
private:
	std::ofstream f_;
	std::mutex mu_;
};