#include "camera_runner.hpp"
#include <vector>
#include <string>
#include <filesystem>




namespace sca {

	enum eInput
	{
		Default = -1,
		eI_Terminate = 0,
		eI_Wait = 1,
		eI_Action = 2
	};
	enum eOutput
	{
		eO_Terminate = 0,
		eO_Wait = 1,
		eO_Action = 2,
		eO_True = 3,
		eO_False = 4
	};
	struct CamConfig {
		eInput inputStep;
		eInput curStep;
		ProcessHandle python_Handle;
	};
	enum class eFile { Auth, Drive,Input,Output,Data };
	const char* ai_filename(eFile type);
	bool cam_initial_(bool type);
	bool cam_Terminate_();

	bool cam_data_setting_(std::pair<uint32_t,float>* data, int len);

	uint8_t cam_authenticating_(bool* ok);

	bool cam_clean_();

	bool cam_start_();

}