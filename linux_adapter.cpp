#include "linux_adapter.hpp"

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <cstdio>

static can_err_t linux_probe(Adapter* /*self*/) {
    // 특별히 할 건 없음. 성공 가정.
    return CAN_OK;
}

static int open_bind_socket(const char* ifname, int* out_fd) {
    int fd = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0) return -1;

    // 넌블로킹
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) { ::close(fd); return -2; }

    sockaddr_can addr{};
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) { ::close(fd); return -3; }

    *out_fd = fd;
    return 0;
}

static void rx_loop(LinuxPriv* p) {
    while (!p->stop.load()) {
        struct can_frame fr{};
        ssize_t n = ::read(p->fd, &fr, sizeof(fr));
        if (n == (ssize_t)sizeof(fr)) {
            if (p->on_rx) {
                CanFrame cf{};
                cf.id  = fr.can_id & CAN_EFF_MASK; // 확장/표준 구분은 생략(마스크)
                cf.dlc = (uint8_t)fr.can_dlc;
                std::memcpy(cf.data, fr.data, cf.dlc);
                p->on_rx(&cf, p->on_rx_user);
            }
        } else {
            // EAGAIN 이면 잠깐 쉼
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(1000 * 1);
            } else {
                // 치명 에러 아닐 땐 그냥 이어감
                usleep(1000 * 5);
            }
        }
    }
}

static can_err_t linux_ch_open(Adapter* self, const char* name, const CanConfig* /*cfg*/, AdapterHandle* out_h) {
    if (!self || !name || !out_h) return CAN_ERR_INVALID;
    auto* priv = new LinuxPriv();

    // name == "can0" 같은 SocketCAN 인터페이스명
    int fd = -1;
    if (open_bind_socket(name, &fd) != 0) {
        delete priv;
        return CAN_ERR_IO;
    }
    priv->fd = fd;

    // RX 스레드 시작
    priv->stop.store(false);
    priv->rx_thread = std::thread(rx_loop, priv);

    self->priv = priv;
    *out_h = (AdapterHandle)priv; // 채널 1:1 가정
    return CAN_OK;
}

static void linux_ch_close(Adapter* self, AdapterHandle h) {
    (void)h;
    if (!self || !self->priv) return;
    auto* p = (LinuxPriv*)self->priv;
    p->stop.store(true);
    if (p->rx_thread.joinable()) p->rx_thread.join();
    if (p->fd >= 0) ::close(p->fd);
    delete p;
    self->priv = nullptr;
}

static can_err_t linux_ch_set_callbacks(
    Adapter* self, AdapterHandle /*h*/,
    adapter_rx_cb_t on_rx, void* on_rx_user,
    adapter_err_cb_t on_err, void* on_err_user,
    adapter_bus_cb_t on_bus, void* on_bus_user)
{
    if (!self || !self->priv) return CAN_ERR_INVALID;
    auto* p = (LinuxPriv*)self->priv;
    p->on_rx       = on_rx;
    p->on_rx_user  = on_rx_user;
    p->on_err      = on_err;
    p->on_err_user = on_err_user;
    p->on_bus      = on_bus;
    p->on_bus_user = on_bus_user;
    return CAN_OK;
}

static can_err_t linux_write(Adapter* self, AdapterHandle /*h*/, const CanFrame* cf, uint32_t /*timeout_ms*/) {
    if (!self || !self->priv || !cf) return CAN_ERR_INVALID;
    auto* p = (LinuxPriv*)self->priv;
    struct can_frame fr{};
    fr.can_id  = cf->id & CAN_EFF_MASK;  // 단순화(표준/확장 플래그는 생략)
    fr.can_dlc = cf->dlc;
    std::memcpy(fr.data, cf->data, fr.can_dlc);

    ssize_t w = ::write(p->fd, &fr, sizeof(fr));
    if (w != (ssize_t)sizeof(fr)) return CAN_ERR_IO;
    return CAN_OK;
}

static can_err_t linux_read(Adapter* self, AdapterHandle /*h*/, CanFrame* out, uint32_t timeout_ms) {
    if (!self || !self->priv || !out) return CAN_ERR_INVALID;
    auto* p = (LinuxPriv*)self->priv;

    // 논블로킹 폴로직(간단)
    uint32_t waited = 0;
    while (waited <= timeout_ms) {
        struct can_frame fr{};
        ssize_t n = ::read(p->fd, &fr, sizeof(fr));
        if (n == (ssize_t)sizeof(fr)) {
            out->id  = fr.can_id & CAN_EFF_MASK;
            out->dlc = (uint8_t)fr.can_dlc;
            std::memcpy(out->data, fr.data, out->dlc);
            return CAN_OK;
        }
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            return CAN_ERR_IO;
        }
        usleep(1000 * 1);
        waited += 1;
    }
    return CAN_ERR_TIMEOUT;
}

static can_bus_state_t linux_status(Adapter* /*self*/, AdapterHandle /*h*/) {
    // 간단화: 항상 ERROR_ACTIVE로 보고
    return CAN_BUS_STATE_ERROR_ACTIVE;
}

static can_err_t linux_recover(Adapter* /*self*/, AdapterHandle /*h*/) {
    // SocketCAN에서는 bus-off 복구를 netlink/ioctl로 하기도 하지만,
    // 간단 구현에선 OK만 리턴
    return CAN_OK;
}

static void linux_destroy(Adapter* self) {
    if (!self) return;
    delete self;
}

static AdapterVTable V = {
    .probe         = linux_probe,
    .ch_open       = linux_ch_open,
    .ch_close      = linux_ch_close,
    .ch_set_callbacks = linux_ch_set_callbacks,
    .write         = linux_write,
    .read          = linux_read,
    .status        = linux_status,
    .recover       = linux_recover,
    .destroy       = linux_destroy
};

Adapter* create_linux_adapter() {
    auto* a = new Adapter();
    a->v    = &V;
    a->priv = nullptr;
    return a;
}
