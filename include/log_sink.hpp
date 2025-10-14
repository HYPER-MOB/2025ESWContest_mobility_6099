#pragma once
#include <string>
#include <fstream>
#include <mutex>


class LogSink {
public:
	bool open(const std::string& path); ///< ���� ����(append)
	void write(const std::string& line);///< Ÿ�ӽ����� + ���� ���
private:
	std::ofstream f_;
	std::mutex mu_;
};