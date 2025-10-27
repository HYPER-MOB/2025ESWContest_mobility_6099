#include "camera_adapter.hpp"
#include "camera_runner.hpp"
#include <filesystem>
#include <utility> 
#include <cstdint>


namespace sca {

	CamConfig cfg;
	bool cam_initial_(eFile type)
	{
		cfg.curStep=eInput::Default;
		cfg.inputStep=eInput::Default;
		cfg.result = false;

		PATH_AI = get_ai_path();
		std::filesystem::path script = PATH_AI / ai_filename(type);
		std::filesystem::path in = PATH_AI / ai_filename(eFile::Input);
		bool ok = write_text(in.string(), "1", WriteMode::Truncate);
		if(ok)
		{	
			cfg.python_Handle = run_python(script.string());
			return cfg.python_Handle.valid();
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
    // 기본값
    if (ok) *ok = false;

    // 아직 종료 안 됨 → 러너가 실행 중이므로 '대기' 상태를 돌려준다.
    int ec = 0;
    if (!try_get_exit(cfg.python_Handle, ec)) {
        return eStatus::Wait; // ★ 기존 Error → Wait 로 교정
    }

    // 종료됨 → 파일에서 상태코드 읽기
    const std::filesystem::path out = PATH_AI / ai_filename(eFile::Output);
    const int val = read_single_char_code(out.string());
    if (val < 0) {
        return eStatus::Error;
    }

    switch ((val - '0')) {
    case eOutput::Terminate:
        if (cfg.inputStep == eInput::Terminate) {
            cam_clean_();
            cfg.curStep = eInput::Terminate;
            if (ok) *ok = cfg.result;
            return eStatus::Terminate;
        }
        break;

    case eOutput::Wait:
        if (cfg.inputStep == eInput::Wait) {
            cfg.curStep = eInput::Wait;
            return eStatus::Ready;
        }
        break;

    case eOutput::Action:
        if (cfg.inputStep == eInput::Action) {
            cfg.curStep = eInput::Action;
            return eStatus::Action;
        }
        break;

    case eOutput::True:
        if (cfg.curStep == eInput::Action) {
            cfg.result = true;
            if (ok) *ok = true;
            return eStatus::Result;
        }
        break;

    case eOutput::False: // ★ enum 이름 통일(대소문자)
        if (cfg.curStep == eInput::Action) {
            cfg.result = false;
            if (ok) *ok = false;
            return eStatus::Result;
        }
        break;

    default:
        break;
    }
    return eStatus::Wait;
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
				usleep(100000); // 100ms
			}
			terminate(cfg.python_Handle, 9);
			int ec = 0; try_get_exit(cfg.python_Handle, ec);
			cfg.python_Handle = ProcessHandle{};
		}

		return true;
	}

}
