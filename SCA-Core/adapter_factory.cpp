#include "adapter.hpp"
#include "can_api.hpp"

// 새로 만든 리눅스 어댑터
#include "linux_adapter.hpp"

// (이미 있던) 디버그 어댑터가 있다면 여기에 include
// #include "adapters/debug_adapter.hpp"

Adapter* create_adapter(can_device_t device){
    switch (device){
    case CAN_DEVICE_LINUX:
        return create_linux_adapter();
    // case CAN_DEVICE_DEBUG:
    //     return create_debug_adapter();
    default:
        return nullptr;
    }
}
