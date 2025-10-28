#include "camera_runner.hpp"

#include <fstream>
#include <filesystem>
#include <iomanip>
#include <locale>
#include <system_error>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <spawn.h>
#include <signal.h>   // for kill()
#include <cstdlib>    // for std::getenv
extern char** environ;

namespace fs = std::filesystem;

namespace sca {

    std::filesystem::path get_ai_path() {
        char buf[4096];
        ssize_t n = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        fs::path start = (n > 0) ? fs::path(std::string(buf, n)).parent_path()
            : fs::current_path();

        for (auto cur = start; !cur.empty(); cur = cur.parent_path()) {
            if (cur.filename() == "SCA-Core") {
                return cur.parent_path() / "SCA-AI"; // work/SCA-AI
            }
        }

        fs::path cwd = fs::current_path();
        if (cwd.filename() == "SCA-Core") {
            return cwd.parent_path() / "SCA-AI";
        }
        return cwd.parent_path() / "SCA-AI";
    }

    static bool ensure_parent_dir(const std::string& path) {
        std::error_code ec;
        fs::path p(path);
        auto parent = p.parent_path();
        if (!parent.empty() && !fs::exists(parent, ec)) {
            fs::create_directories(parent, ec);
            if (ec) return false;
        }
        return true;
    }

    bool write_text(const std::string& path,
        const std::string& data,
        WriteMode mode,
        std::optional<size_t> len)
    {
        if (!ensure_parent_dir(path)) return false;

        std::ios_base::openmode m = std::ios::out;
        m |= (mode == WriteMode::Append) ? std::ios::app : std::ios::trunc;

        std::ofstream ofs(path, m);
        if (!ofs) return false;

        if (len.has_value()) ofs.write(data.data(), static_cast<std::streamsize>(*len));
        else                 ofs.write(data.data(), static_cast<std::streamsize>(data.size()));

        return static_cast<bool>(ofs);
    }

    bool write_pair_line(const std::string& path,
        uint32_t u, float f,
        WriteMode mode,
        int precision)
    {
        if (!ensure_parent_dir(path)) return false;

        std::ios_base::openmode m = std::ios::out;
        m |= (mode == WriteMode::Append) ? std::ios::app : std::ios::trunc;

        std::ofstream ofs(path, m);
        if (!ofs) return false;

        ofs.imbue(std::locale::classic());
        ofs << std::fixed << std::setprecision(precision);
        ofs << u << ' ' << f << '\n';
        return static_cast<bool>(ofs);
    }

    int read_single_char_code(const std::string& path)
    {
        std::ifstream ifs(path);
        if (!ifs) return -1;

        ifs >> std::ws;
        int ch = ifs.get();
        if (ch == EOF) return -1;

        unsigned char uc = static_cast<unsigned char>(ch);
        return static_cast<int>(uc);
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // 기존 make_argv (사용하지 않을 수도 있지만 남겨둠)
    static std::vector<char*> make_argv(const std::string& prog,
        const std::vector<std::string>& args)
    {
        std::vector<char*> v;
        v.reserve(2 + args.size());
        v.push_back(const_cast<char*>(prog.c_str()));
        for (const auto& s : args) v.push_back(const_cast<char*>(s.c_str()));
        v.push_back(nullptr);
        return v;
    }

    // 환경 변수 헬퍼
    static inline const char* getenv_or_null(const char* k) {
        const char* v = std::getenv(k);
        return (v && *v) ? v : nullptr;
    }

    struct PythonCommand {
        // exe: 실행 파일 이름 또는 절대경로 (예: "/home/pi/miniforge3/envs/mediapipe/bin/python" 또는 "conda")
        // argv: exe 이후 인자들 (예: {"run","-n","mediapipe","python","<script>", ...})
        std::string exe;
        std::vector<std::string> argv;
    };

    // 우선순위: SCA_PYTHON(절대경로) → SCA_CONDA_ENV(환경 이름) → "python3"
    static PythonCommand build_python_command(const std::filesystem::path& script,
        const std::vector<std::string>& passthru_args)
    {
        if (auto p = getenv_or_null("SCA_PYTHON")) {
            PythonCommand cmd;
            cmd.exe = p; // 절대경로 python
            cmd.argv.push_back(script.string());
            cmd.argv.insert(cmd.argv.end(), passthru_args.begin(), passthru_args.end());
            return cmd;
        }
        if (auto env = getenv_or_null("SCA_CONDA_ENV")) {
            PythonCommand cmd;
            cmd.exe = "conda";
            cmd.argv = { "run", "-n", env, "python", script.string() };
            cmd.argv.insert(cmd.argv.end(), passthru_args.begin(), passthru_args.end());
            return cmd;
        }
        // default: system python3
        PythonCommand cmd;
        cmd.exe = "python3";
        cmd.argv.push_back(script.string());
        cmd.argv.insert(cmd.argv.end(), passthru_args.begin(), passthru_args.end());
        return cmd;
    }

    static std::vector<char*> make_argv_execform(const std::string& exe,
        const std::vector<std::string>& args)
    {
        std::vector<char*> v;
        v.reserve(2 + args.size());
        v.push_back(const_cast<char*>(exe.c_str()));      // argv[0] = exe
        for (auto const& s : args) v.push_back(const_cast<char*>(s.c_str()));
        v.push_back(nullptr);
        return v;
    }
    // ─────────────────────────────────────────────────────────────────────────────

    ProcessHandle run_python(const std::string& script_filename,
        const std::vector<std::string>& args,
        const std::optional<std::string>& working_dir)
    {
        // 스크립트 경로 해석 (working_dir이 있으면 그 기준으로 붙임)
        std::filesystem::path script = working_dir
            ? (std::filesystem::path(*working_dir) / script_filename)
            : std::filesystem::path(script_filename);

        // 어떤 방식으로 python을 실행할지 결정
        PythonCommand pcmd = build_python_command(script, args);

        // argv 구성
        std::vector<char*> argv = make_argv_execform(pcmd.exe, pcmd.argv);

        // working_dir 없으면 posix_spawnp로 바로 실행
        if (!working_dir.has_value() || working_dir->empty()) {
            pid_t pid = -1;
            int rc = posix_spawnp(&pid, pcmd.exe.c_str(),
                /*file_actions*/nullptr,
                /*attrp*/nullptr,
                argv.data(), environ);
            if (rc != 0) {
                return ProcessHandle{ -1 };
            }
            return ProcessHandle{ static_cast<int>(pid) };
        }

        // working_dir 있으면 fork + chdir + execvp
        pid_t pid = ::fork();
        if (pid < 0) {
            // fork 실패
            return ProcessHandle{ -1 };
        }
        if (pid == 0) {
            // 자식 프로세스
            if (::chdir(working_dir->c_str()) != 0) {
                _exit(127); // chdir 실패
            }
            ::execvp(pcmd.exe.c_str(), argv.data()); // exe가 "conda"일 수도, 절대경로 python일 수도
            _exit(127); // exec 실패 시
        }
        // 부모: 자식 PID 반환
        return ProcessHandle{ static_cast<int>(pid) };
    }

    bool try_get_exit(const ProcessHandle& h, int& exit_code)
    {
        if (!h.valid()) return false;

        int status = 0;
        pid_t r = waitpid(h.pid, &status, WNOHANG);
        if (r == 0) {
            return false;
        }
        if (r == h.pid) {
            if (WIFEXITED(status)) {
                exit_code = WEXITSTATUS(status);
            }
            else if (WIFSIGNALED(status)) {
                exit_code = -WTERMSIG(status);
            }
            else {
                exit_code = -999;
            }
            return true;
        }
        return false;
    }

    bool is_running(const ProcessHandle& h)
    {
        if (!h.valid()) return false;
        int status = 0;
        pid_t r = waitpid(h.pid, &status, WNOHANG);
        return (r == 0);
    }

    bool terminate(const ProcessHandle& h, int sig)
    {
        if (!h.valid()) return false;
        return (::kill(h.pid, sig) == 0);
    }

} // namespace sca
