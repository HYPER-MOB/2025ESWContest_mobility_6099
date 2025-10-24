#include "ble_adapter.hpp"
// 네 BLE 구현 헤더(함수 시그니처를 아래처럼 하나만 공개하도록 해둔 상태라고 가정)
#include "sca_ble_peripheral.hpp"

// sca_ble_peripheral.cpp 내부에서 아래 함수만 외부로 export 하도록 구성:
// bool sca_ble_run_gatt(const std::string& last12,
//                       const std::string& local_name,
//                       int timeout_s,
//                       const std::string& expected_ascii);

extern bool sca_ble_run_gatt(const std::string&,
                             const std::string&,
                             int,
                             const std::string&);

bool sca_ble_advertise_and_wait(const std::string& last12,
                                const std::string& local_name,
                                int timeout_s,
                                const std::string& expected_ascii)
{
    return sca_ble_run_gatt(last12, local_name, timeout_s, expected_ascii);
}
