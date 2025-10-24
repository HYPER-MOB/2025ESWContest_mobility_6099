#pragma once
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>

struct CANBus {
    int fd = -1;

    bool open(const char* ifname) {
        fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (fd < 0) return false;
        struct ifreq ifr {}; std::snprintf(ifr.ifr_name, IFNAMSIZ, "%s", ifname);
        if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) { ::close(fd); fd = -1; return false; }
        sockaddr_can addr{}; addr.can_family = AF_CAN; addr.can_ifindex = ifr.ifr_ifindex;
        if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) { ::close(fd); fd = -1; return false; }
        return true;
    }
    bool send(uint32_t id, const uint8_t* data, uint8_t dlc) {
        struct can_frame f {}; f.can_id = id; f.can_dlc = dlc;
        if (dlc) std::memcpy(f.data, data, dlc);
        return ::write(fd, &f, sizeof(f)) == (ssize_t)sizeof(f);
    }
    bool recv(struct can_frame& f) {
        ssize_t n = ::read(fd, &f, sizeof(f));
        return n == (ssize_t)sizeof(f);
    }
    void close() { if (fd >= 0) ::close(fd), fd = -1; }
    ~CANBus() { close(); }
};
