#include "camera_runner.hpp"
#include <vector>
#include <string>


#defien CAM_AUTH ""
#define CAM_RIDE ""
#define CAM_DATA ""
#define CAM_INPUT ""
#define CAM_OUTPUT ""


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
		FALSE =4
	};
	bool cam_initial_();

	bool cam_data_setting_(std::pair<uint32_t,float>* data, int len);

	bool cam_authenticating_();

	bool cam_clean_();

	bool cam_start_();

	std::filesystem PATH_AI;
	ProcessHandle python_Handle;

}