#include "nfc_module.hpp"
#include "event_bus.hpp"
#include <nlohmann/json.hpp>
#include <thread>
#include <atomic>


static std::atomic<bool> g_nfc_run{ false };


bool NFCModule::init() { return true; }


bool NFCModule::pollStart() {
	g_nfc_run = true;
	std::thread([] {
		using json = nlohmann::json;
		EventBus bus;
		while (g_nfc_run) {
			std::this_thread::sleep_for(std::chrono::seconds(10));
			json j{ {"type","nfc_ok"}, {"uid","DEMOUID"}, {"token",""} };
			bus.send("/tmp/sca_uds", j);
		}
		}).detach();
	return true;
}


void NFCModule::pollStop() { g_nfc_run = false; }