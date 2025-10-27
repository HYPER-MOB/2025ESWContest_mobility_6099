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

        // 공백 제거 후 한 문자만
        ifs >> std::ws;
        int ch = ifs.get();
        if (ch == EOF) return -1;

        // unsigned char 로 정규화한 값을 int로
        unsigned char uc = static_cast<unsigned char>(ch);
        return static_cast<int>(uc);
    }

    // ----- Python process -----

    // argv 벡터를 posix_spawnp에 맞게 변환
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

    ProcessHandle run_python(const std::string& script_filename,
        const std::vector<std::string>& args,
        const std::optional<std::string>& working_dir)
    {
        const std::string python = "python3";

        // 절대/상대 경로 모두 허용. chdir 하지 않고 그대로 실행.
        std::vector<std::string> all_args;
        all_args.reserve(1 + args.size());
        all_args.emplace_back(script_filename);
        all_args.insert(all_args.end(), args.begin(), args.end());

        pid_t pid = -1;
        std::vector<char*> argv = make_argv(python, all_args);

        posix_spawn_file_actions_t fa;
        posix_spawn_file_actions_t* pfa = nullptr;

        posix_spawn_file_actions_init(&fa);
        if (working_dir.has_value()) {
            // working_dir이 있으면 chdir을 파일액션으로 적용 (프로세스 전역 chdir 회피)
            // posix_spawn에는 직접 chdir 액션이 없어 파이썬 쪽에서 처리하거나
            // script_filename을 working_dir/script로 구성하는 것이 일반적.
            // 여기서는 간단히 절대경로 생성으로 대체 권장 → working_dir는 무시
        }
        // pfa = &fa; // (위 주석처럼 절대경로 사용 권장이라 실제 적용은 생략)

        int rc = posix_spawnp(&pid, python.c_str(), pfa,
            /*attrp*/nullptr, argv.data(), environ);
        posix_spawn_file_actions_destroy(&fa);

        if (rc != 0) {
            return ProcessHandle{ -1 };
        }
        return ProcessHandle{ static_cast<int>(pid) };
    }

    bool try_get_exit(const ProcessHandle& h, int& exit_code)
    {
        if (!h.valid()) return false;

        int status = 0;
        pid_t r = waitpid(h.pid, &status, WNOHANG);
        if (r == 0) {
            // 아직 실행 중
            return false;
        }
        if (r == h.pid) {
            if (WIFEXITED(status)) {
                exit_code = WEXITSTATUS(status);
            }
            else if (WIFSIGNALED(status)) {
                // 시그널로 종료된 경우 음수로 표기
                exit_code = -WTERMSIG(status);
            }
            else {
                exit_code = -999; // 기타
            }
            return true;
        }
        // waitpid 에러
        return false;
    }

    bool is_running(const ProcessHandle& h)
    {
        if (!h.valid()) return false;
        int status = 0;
        pid_t r = waitpid(h.pid, &status, WNOHANG);
        return (r == 0); // 0이면 아직 실행 중
    }

    bool terminate(const ProcessHandle& h, int sig)
    {
        if (!h.valid()) return false;
        return (::kill(h.pid, sig) == 0);
    }

} // namespace sca
