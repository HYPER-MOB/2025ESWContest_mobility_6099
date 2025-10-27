#include "camera_adapter.hpp"
#include "camera_runner.hpp"
#include <filesystem>
#include <utility> 
#include <cstdint>

namespace sca {

	bool cam_initial_(eFile type)
	{
		PATH_AI = get_ai_path();
		fs::path script = PATH_AI / ai_filename(type);
		python_Handle = run_python(script.string());
		return python_Handle.valid();
	}
	bool cam_data_setting_(std::pair<uint32_t, float>* data, int len)
	{


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
		fs::path in = PATH_AI / ai_filename(eFile::Input);
		return write_text(in.string(), "1", WriteMode::Truncate);
	}

	bool cam_authenticating_() {
		int ec;
		if (try_get_exit(python_Handle, ec)) {
			//의도인지 파일읽어서 확인할 예정.
			// 의도치 않는 종료
			cam_clean_();
			return false;
			// 종료됨. ec=0 정상
		}
		else {
			// 아직 실행 중
			return true;
		}
	}

	bool cam_clean_() {
		if (python_Handle.valid()) {
			terminate(python_Handle, 15);
			for (int i = 0; i < 20; ++i) {
				int ec = 0;
				if (try_get_exit(python_Handle, ec)) {
					python_Handle = ProcessHandle{}; 
					return true;
				}
				usleep(100000); // 100ms
			}
			terminate(python_Handle, 9);
			int ec = 0; try_get_exit(python_Handle, ec);
			python_Handle = ProcessHandle{};
		}

		return true;
	}

}