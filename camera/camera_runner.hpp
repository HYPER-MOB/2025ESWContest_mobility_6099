#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <filesystem>

namespace sca {
    enum class WriteMode { Append, Truncate };
    std::filesystem::path get_ai_path();
    bool write_text(const std::string& path,
        const std::string& data,
        WriteMode mode = WriteMode::Append,
        std::optional<size_t> len = std::nullopt);

    bool write_pair_line(const std::string& path,
        uint32_t u, float f,
        WriteMode mode = WriteMode::Append,
        int precision = 6);

    int read_single_char_code(const std::string& path);

    struct ProcessHandle {
        int pid{ -1 };
        bool valid() const { return pid > 0; }
    };

    ProcessHandle run_python(const std::string& script_filename,
        const std::vector<std::string>& args = {},
        const std::optional<std::string>& working_dir = std::nullopt);

    bool try_get_exit(const ProcessHandle& h, int& exit_code);
    bool is_running(const ProcessHandle& h);

    bool terminate(const ProcessHandle& h, int sig = 15 /*SIGTERM*/);


    bool cam_data_save();
}