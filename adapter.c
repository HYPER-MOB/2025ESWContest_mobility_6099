#include "adapter.h"
#include <stdlib.h>
#include <string.h>

#if defined(__linux__)
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

typedef struct {
    int fd; // per-channel
} LinChan;

static can_err_t probe(Adapter* self) {
    (void)self;
    int fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0) {
        switch (errno) {
            case EACCES:
            case EPERM: return CAN_ERR_PERMISSION;
            case EAFNOSUPPORT:
            case EPROTONOSUPPORT: return CAN_ERR_NODEV;
            default: return CAN_ERR_IO;
        }
    }
    close(fd);
    return CAN_OK;
}

static can_err_t ch_open(Adapter* self, const char* ifname, const CanConfig* cfg, void** out) {
    (void)self; (void)cfg;
    if (!ifname || !out) return CAN_ERR_INVALID;
    int fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0) return CAN_ERR_IO;

    struct ifreq ifr = {0};
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name)-1);
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) { int e=errno; close(fd); return (e==ENODEV)?CAN_ERR_NODEV:CAN_ERR_IO; }

    struct sockaddr_can addr = {0};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { int e=errno; close(fd); return (e==ENODEV)?CAN_ERR_NODEV:CAN_ERR_IO; }

    LinChan* h = (LinChan*)calloc(1, sizeof(LinChan));
    if (!h) { close(fd); return CAN_ERR_MEMORY; }
    h->fd = fd;
    *out = h;
    return CAN_OK;
}

static void ch_close(Adapter* self, void* handle) {
    (void)self;
    LinChan* h = (LinChan*)handle;
    if (!h) return;
    if (h->fd >= 0) close(h->fd);
    free(h);
}

static can_err_t map_errno(int e) {
    switch (e) {
        case EAGAIN:
        case EWOULDBLOCK: return CAN_ERR_AGAIN;
        case ENETDOWN:    return CAN_ERR_STATE;
        case EPERM:
        case EACCES:      return CAN_ERR_PERMISSION;
        case ENODEV:
        case ENXIO:       return CAN_ERR_NODEV;
        default:          return CAN_ERR_IO;
    }
}

static can_err_t poll_wait(int fd, short events, uint32_t timeout_ms) {
    struct pollfd pfd = { .fd=fd, .events=events, .revents=0 };
    int r = poll(&pfd, 1, (int)timeout_ms);
    if (r == 0) return CAN_ERR_TIMEOUT;
    if (r < 0)  return map_errno(errno);
    if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) return CAN_ERR_IO;
    return CAN_OK;
}

static can_err_t to_linux(const CanFrame* in, struct can_frame* out) {
    if (!in || !out) return CAN_ERR_INVALID;
    canid_t id = 0;
    if (in->flags & CAN_FRAME_EXTID) {
        if (in->id > CAN_EFF_MASK) return CAN_ERR_INVALID;
        id = (in->id & CAN_EFF_MASK) | CAN_EFF_FLAG;
    } else {
        if (in->id > CAN_SFF_MASK) return CAN_ERR_INVALID;
        id = (in->id & CAN_SFF_MASK);
    }
    if (in->flags & CAN_FRAME_RTR) id |= CAN_RTR_FLAG;
    if (in->flags & CAN_FRAME_ERR) id |= CAN_ERR_FLAG;
    out->can_id = id;
    out->can_dlc = (in->dlc > 8) ? 8 : in->dlc;
    if (out->can_dlc) memcpy(out->data, in->data, out->can_dlc);
    if (out->can_dlc < 8) memset(out->data + out->can_dlc, 0, 8 - out->can_dlc);
    return CAN_OK;
}

static void from_linux(const struct can_frame* in, CanFrame* out) {
    out->flags = 0;
    if (in->can_id & CAN_EFF_FLAG) { out->flags |= CAN_FRAME_EXTID; out->id = in->can_id & CAN_EFF_MASK; }
    else                            { out->id = in->can_id & CAN_SFF_MASK; }
    if (in->can_id & CAN_RTR_FLAG)  out->flags |= CAN_FRAME_RTR;
    if (in->can_id & CAN_ERR_FLAG)  out->flags |= CAN_FRAME_ERR;
    out->can_dlc = (in->can_dlc > 8) ? 8 : in->can_dlc;
    memcpy(out->data, in->data, out->can_dlc);
    if (out->can_dlc < 8) memset(out->data + out->can_dlc, 0, 8 - out->can_dlc);
}

static can_err_t write(Adapter* self, void* handle, const CanFrame* fr, uint32_t timeout_ms) {
    (void)self;
    LinChan* h = (LinChan*)handle;
    if (!h || h->fd < 0) return CAN_ERR_STATE;

    struct can_frame lf;
    can_err_t e = to_linux(fr, &lf);
    if (e != CAN_OK) return e;

    e = poll_wait(h->fd, POLLOUT, timeout_ms);
    if (e != CAN_OK) return e;

    ssize_t n = send(h->fd, &lf, sizeof(lf), 0);
    if (n < 0) return map_errno(errno);
    if ((size_t)n != sizeof(lf)) return CAN_ERR_IO;
    return CAN_OK;
}

static can_err_t read(Adapter* self, void* handle, CanFrame* out, uint32_t timeout_ms) {
    (void)self;
    LinChan* h = (LinChan*)handle;
    if (!h || h->fd < 0) return CAN_ERR_STATE;

    can_err_t e = poll_wait(h->fd, POLLIN, timeout_ms);
    if (e != CAN_OK) return e;

    struct can_frame lf;
    ssize_t n = recv(h->fd, &lf, sizeof(lf), 0);
    if (n < 0) return map_errno(errno);
    if ((size_t)n != sizeof(lf)) return CAN_ERR_IO;

    from_linux(&lf, out);
    return CAN_OK;
}

static can_bus_state_t status(Adapter* self, void* handle) {
    (void)self; (void)handle;
    // 간단 구현: 상세 상태는 netlink/ioctl 확장 가능
    return CAN_BUS_STATE_ERROR_ACTIVE;
}

static can_err_t recover(Adapter* self, void* handle) {
    (void)self; (void)handle;
    // SocketCAN은 유저 공간에서 특별한 복구 없음(링크 down/up은 netlink 필요)
    return CAN_OK;
}

static void destroy(Adapter* self) {
    free(self);
}

/* -------- Adapter 팩토리 -------- */
static const AdapterVTable VT = {
    .probe      = probe,
    .ch_open    = ch_open,
    .ch_close   = ch_close,
    .write      = write,
    .read       = read,
    .status     = status,
    .recover    = recover,
    .destroy    = destroy
};

static Adapter* linux_adapter_new(void) {
    Adapter* a = (Adapter*)calloc(1, sizeof(Adapter));
    if (!a) return NULL;
    a->v = &VT;
    return a;
}
#endif /* __linux__ */

Adapter* create_adapter(can_device_t device) {
#if defined(__linux__)
    if(device == CAN_DEVICE_LINUX) return linux_adapter_new();
#endif
    return NULL;
}