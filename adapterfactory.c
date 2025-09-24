#include "adapter.h"

Adapter* adapter_debug_new(void);
Adapter* adapter_linux_new(void);
Adapter* adapter_esp32_new(void);

Adapter* create_adapter(can_device_t device) {
    switch (device) {
        //case CAN_DEVICE_LINUX:  return adapter_linux_new();
        case CAN_DEVICE_ESP32:  return adapter_esp32_new();
        //case CAN_DEVICE_DEBUG:  return adapter_debug_new();
        default:                return NULL;           
    }
}