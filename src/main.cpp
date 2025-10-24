#include "sequencer.hpp"
#include "can_api.hpp"
#include <iostream>

int main() {
    if (can_init(CAN_DEVICE_LINUX) != CAN_OK) {
        std::cerr << "can_init failed\n"; return 1;
    }
    CanConfig cfg{}; cfg.mode = CAN_MODE_NORMAL; cfg.bitrate = 500000;
    if (can_open("can0", cfg) != CAN_OK) {
        std::cerr << "can_open(can0) failed\n"; return 1;
    }

    Sequencer seq;
    seq.start();

    std::cout << "[main] CAN ready. Waiting frames…\n";
    while (true) {
        CanFrame f{};
        can_err_t r = can_recv("can0", &f, 0xFFFFFFFFu);
        if (r != CAN_OK) continue;
        seq.on_can_frame(f);
    }

    // 도달하지 않지만, 구조상:
    // seq.stop();
    // can_dispose();
    // return 0;
}
