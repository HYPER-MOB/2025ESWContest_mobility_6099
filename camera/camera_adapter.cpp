#include "camera_adapter.hpp"
#include "camera_runner.hpp"
#include <filesystem>
#include <utility> 
#include <cstdint>
#include <chrono>
#include <thread>

#define CAM_AUTH ""
#define CAM_RIDE ""
#define CAM_DATA "user1.txt"
#define CAM_INPUT "input.txt"
#define CAM_OUTPUT "output.txt"
namespace sca {
	CamConfig cfg; std::filesystem::path PATH_AI;

	const char* ai_filename(eFile type) {
		switch (type) {
		case eFile::Auth: return CAM_AUTH;
		case eFile::Drive: return CAM_RIDE;
		case eFile::Data: return CAM_DATA;
		case eFile::Input: return CAM_INPUT;
		case eFile::Output: return CAM_OUTPUT;
		default:          return CAM_AUTH;
		}
	}
	bool cam_initial_(bool type)
	{
		cfg.curStep=eInput::Default;
		cfg.inputStep=eInput::Default;
		cfg.result = false;

		PATH_AI = get_ai_path();
		std::filesystem::path script = PATH_AI / ai_filename(type?eFile::Auth:eFile::Drive);
		std::filesystem::path in = PATH_AI / ai_filename(eFile::Input);
		bool ok = write_text(in.string(), "1", WriteMode::Truncate);
		if(ok)
		{
			cfg.python_Handle = run_python(script.string());
			ok = cfg.python_Handle.valid();
			if (ok) cfg.inputStep = eInput::eI_Wait;
			else std::printf("[CAM] Program error\n");
			return ok;
		}
		return false;// not write input txt
	}
	bool cam_data_setting_(std::pair<uint32_t, float>* data, int len)
	{
		std::filesystem::path dst= PATH_AI / ai_filename(eFile::Data);

		if (len > 0) {
			if (!write_pair_line(dst.string(), data[0].first, data[0].second,
				WriteMode::Truncate, 6)) return false;
		}
		for (int i = 1; i < len; ++i) {
			if (!write_pair_line(dst.string(), data[i].first, data[i].second,
				WriteMode::Append, 6)) return false;
		}

		return true;
	}
	bool cam_start_()
	{
		if(cfg.curStep!=eInput::Wait)return false;
		std::filesystem::path in = PATH_AI / ai_filename(eFile::Input);
		bool ok = write_text(in.string(), "2", WriteMode::Truncate);
		if(ok)cfg.inputStep = eInput::Action;
		return ok;
	}

	bool cam_Terminate_()
	{
		std::filesystem::path in = PATH_AI / ai_filename(eFile::Input);
		bool ok = write_text(in.string(), "0", WriteMode::Truncate);
		cfg.inputStep = eInput::Terminate;
		return ok;
	}

	uint8_t cam_authenticating_(bool* ok) {
    if (ok) *ok = false;

    int ec = 0;
    if (!try_get_exit(cfg.python_Handle, ec)) {
        return 0;
    }

    const std::filesystem::path out = PATH_AI / ai_filename(eFile::Output);
    const char* val = read_single_char_code(out.string());
    if (val < 0) {
        return 4;
    }

    switch (val) {
    case '0'://Terminate
        if (cfg.inputStep == eInput::Terminate) {
            cam_clean_();
            cfg.curStep = eInput::Terminate;
            return 2;
        }
        break;

    case '1'://Ready
        if (cfg.inputStep == eInput::Wait) {
            cfg.curStep = eInput::Wait;
            return 1;
        }
        break;

    case '2'://Action
        if (cfg.inputStep == eInput::Action) {
			std::printf("[CAM] Action\n");
            cfg.curStep = eInput::Action;
        }
        break;

    case '3'://Action
        if (cfg.curStep == eInput::Action) {
            if (ok) *ok = true;
            return 3;
        }
        break;

	case '4':
        if (cfg.curStep == eInput::Action) {
            if (ok) *ok = false;
			return 3;
        }
        break;

    default:
        break;
    }
    return 0;
}

	bool cam_clean_() {
		if (cfg.python_Handle.valid()) {
			terminate(cfg.python_Handle, 15);
			for (int i = 0; i < 20; ++i) {
				int ec = 0;
				if (try_get_exit(cfg.python_Handle, ec)) {
					cfg.python_Handle = ProcessHandle{}; 
					return true;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

			}
			terminate(cfg.python_Handle, 9);
			int ec = 0; try_get_exit(cfg.python_Handle, ec);
			cfg.python_Handle = ProcessHandle{};
		}

		return true;
	}

}
