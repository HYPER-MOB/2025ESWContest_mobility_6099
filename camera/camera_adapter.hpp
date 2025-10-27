#include "camera_runner.hpp"
#include <vector>
#include <string>
#include <filesystem>


#define CAM_AUTH ""
#define CAM_RIDE ""
#define CAM_DATA "user1.txt"
#define CAM_INPUT "input.txt"
#define CAM_OUTPUT "output.txt"


namespace sca {

	enum class eFile { Auth, Ride,Input,Output,Data };
	const char* ai_filename(eFile type) {
		switch (type) {
		case eFile::Auth: return CAM_AUTH;
		case eFile::Ride: return CAM_RIDE;
		case eFile::Data: return CAM_DATA;
		case eFile::Input: return CAM_INPUT;
		case eFile::Output: return CAM_OUTPUT;
		default:          return CAM_AUTH;
		}
	}
	enum eInput
	{
		Default = -1,
		Terminate = 0,
		Wait = 1,
		Action = 2
	};
	enum eOutput
	{
		Terminate =0,
		Wait = 1,
		Action =2,
		True = 3,
		False =4
	};
	enum eStatus
	{
		Wait =0,
		Action = 1,
		Ready = 2,
		Terminate =3,
		Result = 4,
		Error = 5
	};
	bool cam_initial_();
	bool cam_Terminate_();

	bool cam_data_setting_(std::pair<uint32_t,float>* data, int len);

	uint8_t cam_authenticating_(bool ok);

	bool cam_clean_();

	bool cam_start_();
	std::filesystem::path PATH_AI;
    struct CamConfig {
		eInput inputStep;
		eInput curStep;
		bool result;
		ProcessHandle python_Handle;
    };

}