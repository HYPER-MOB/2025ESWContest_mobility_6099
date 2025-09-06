// adapter_linux.c
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include "adapter.h"

// per-channel
typedef struct {
    int              sock;
    int              ifindex;
    char             ifname[IFNAMSIZ];

    // 콜백
    adapter_rx_cb_t  on_rx;
    void*            on_rx_user;
    adapter_err_cb_t on_err;
    void*            on_err_user;
    adapter_bus_cb_t on_bus;
    void*            on_bus_user;

    // RX 워커
    pthread_t        rx_thread;
    volatile int     running;
    pthread_mutex_t  mtx;
} LinuxCh;

typedef struct {
    // 전역으로 쓸 게 있으면 여기에
    int dummy;
} LinuxPriv;

static can_err_t map_errno(void){
    switch (errno){
        case EAGAIN: return CAN_ERR_AGAIN;
        case ETIMEDOUT: return CAN_ERR_TIMEOUT;
        case EPERM: case EACCES: return CAN_ERR_PERMISSION;
        case ENODEV: return CAN_ERR_NODEV;
        default: return CAN_ERR_IO;
    }
}

static can_err_t get_ifindex(const char* name, int* out_idx){
    int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) return CAN_ERR_IO;
    struct ifreq ifr; memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, name, IFNAMSIZ-1);
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0){ close(s); return CAN_ERR_NODEV; }
    *out_idx = ifr.ifr_ifindex;
    close(s);
    return CAN_OK;
}

static void canframe_from_linux(const struct can_frame* in, CanFrame* out){
    out->id   = (in->can_id & CAN_EFF_FLAG) ? (in->can_id & CAN_EFF_MASK)
                                            : (in->can_id & CAN_SFF_MASK);
    out->dlc  = in->can_dlc;
    out->flags = 0;
    if (in->can_id & CAN_EFF_FLAG) out->flags |= CAN_FRAME_EXTID;
    if (in->can_id & CAN_RTR_FLAG) out->flags |= CAN_FRAME_RTR;
    if (in->can_id & CAN_ERR_FLAG) out->flags |= CAN_FRAME_ERR;
    memcpy(out->data, in->data, 8);
}

static void linux_from_canframe(const CanFrame* in, struct can_frame* out){
    memset(out, 0, sizeof(*out));
    if (in->flags & CAN_FRAME_EXTID) out->can_id = (in->id & CAN_EFF_MASK) | CAN_EFF_FLAG;
    else                             out->can_id = (in->id & CAN_SFF_MASK);
    if (in->flags & CAN_FRAME_RTR)   out->can_id |= CAN_RTR_FLAG;
    if (in->flags & CAN_FRAME_ERR)   out->can_id |= CAN_ERR_FLAG;
    out->can_dlc = (in->dlc > 8) ? 8 : in->dlc;
    memcpy(out->data, in->data, out->can_dlc);
}

static void* rx_worker(void* arg){
    LinuxCh* ch = (LinuxCh*)arg;

    struct pollfd pfd = { .fd = ch->sock, .events = POLLIN };
    while (ch->running){
        int pr = poll(&pfd, 1, 200); // 200ms tick
        if (pr < 0){
            if (errno == EINTR) continue;
            if (ch->on_err) ch->on_err(CAN_ERR_IO, ch->on_err_user);
            break;
        }
        if (pr == 0) continue; // timeout

        if (pfd.revents & POLLIN){
            struct can_frame lfr;
            ssize_t n = read(ch->sock, &lfr, sizeof(lfr));
            if (n == (ssize_t)sizeof(lfr)){
                CanFrame cf; canframe_from_linux(&lfr, &cf);
                adapter_rx_cb_t cb; void* u;
                pthread_mutex_lock(&ch->mtx);
                cb = ch->on_rx; u = ch->on_rx_user;
                pthread_mutex_unlock(&ch->mtx);
                if (cb) cb(&cf, u);
            }
        }
    }
    return NULL;
}

/* ========== VTABLE ========== */

static can_err_t v_probe(Adapter* self){
    (void)self;
    // 간단히 OK (실사용은 ch_open에서 bind로 검증)
    return CAN_OK;
}

static can_err_t v_ch_open(Adapter* self, const char* name, const CanConfig* cfg, AdapterHandle* out_handle){
    (void)cfg;
    if (!name || !out_handle) return CAN_ERR_INVALID;

    int ifindex=0; can_err_t r = get_ifindex(name, &ifindex);
    if (r != CAN_OK) return r;

    int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) return map_errno();

    // (옵션) loopback 끄기/자기 송신 수신 여부
    // int off = 0; setsockopt(s, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &off, sizeof(off));

    struct sockaddr_can addr = {0};
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifindex;
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        int e = errno; close(s); errno = e; return map_errno();
    }

    // 논블로킹은 read()/poll 혼합에 따라 선택
    int fl = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, fl | O_NONBLOCK);

    LinuxCh* ch = (LinuxCh*)calloc(1, sizeof(LinuxCh));
    if (!ch){ close(s); return CAN_ERR_MEMORY; }
    ch->sock = s;
    ch->ifindex = ifindex;
    strncpy(ch->ifname, name, IFNAMSIZ-1);
    pthread_mutex_init(&ch->mtx, NULL);

    ch->running = 1;
    if (pthread_create(&ch->rx_thread, NULL, rx_worker, ch) != 0){
        pthread_mutex_destroy(&ch->mtx);
        close(s);
        free(ch);
        return CAN_ERR_IO;
    }

    *out_handle = (AdapterHandle)ch;
    return CAN_OK;
}

static void v_ch_close(Adapter* self, AdapterHandle h){
    (void)self;
    if (!h) return;
    LinuxCh* ch = (LinuxCh*)h;
    ch->running = 0;
    if (ch->rx_thread) pthread_join(ch->rx_thread, NULL);
    if (ch->sock >= 0) close(ch->sock);
    pthread_mutex_destroy(&ch->mtx);
    free(ch);
}

static can_err_t v_ch_set_callbacks(
    Adapter* self, AdapterHandle h,
    adapter_rx_cb_t on_rx,  void* on_rx_user,
    adapter_err_cb_t on_err, void* on_err_user,
    adapter_bus_cb_t on_bus, void* on_bus_user
){
    (void)self;
    if (!h) return CAN_ERR_INVALID;
    LinuxCh* ch = (LinuxCh*)h;
    pthread_mutex_lock(&ch->mtx);
    ch->on_rx = on_rx; ch->on_rx_user = on_rx_user;
    ch->on_err = on_err; ch->on_err_user = on_err_user;
    ch->on_bus = on_bus; ch->on_bus_user = on_bus_user;
    pthread_mutex_unlock(&ch->mtx);
    return CAN_OK;
}

static can_err_t v_write(Adapter* self, AdapterHandle h, const CanFrame* fr, uint32_t timeout_ms){
    (void)self; (void)timeout_ms;
    if (!h || !fr) return CAN_ERR_INVALID;
    LinuxCh* ch = (LinuxCh*)h;

    struct can_frame lfr;
    linux_from_canframe(fr, &lfr);

    ssize_t n = write(ch->sock, &lfr, sizeof(lfr));
    if (n == (ssize_t)sizeof(lfr)) return CAN_OK;
    return map_errno();
}

static can_err_t v_read(Adapter* self, AdapterHandle h, CanFrame* out, uint32_t timeout_ms){
    (void)self;
    if (!h || !out) return CAN_ERR_INVALID;
    LinuxCh* ch = (LinuxCh*)h;

    struct pollfd pfd = { .fd = ch->sock, .events = POLLIN };
    int to = (timeout_ms == UINT32_MAX) ? -1 : (int)timeout_ms;
    int r = poll(&pfd, 1, to);
    if (r == 0) return CAN_ERR_TIMEOUT;
    if (r < 0) return map_errno();
    if (pfd.revents & POLLIN){
        struct can_frame lfr;
        ssize_t n = read(ch->sock, &lfr, sizeof(lfr));
        if (n == (ssize_t)sizeof(lfr)){
            canframe_from_linux(&lfr, out);
            return CAN_OK;
        }
        return map_errno();
    }
    return CAN_ERR_AGAIN;
}

static can_bus_state_t v_status(Adapter* self, AdapterHandle h){
    (void)self; (void)h;
    // 사용자 공간에서 정확한 bus-off 감지는 제한적. 단순 정상 가정.
    return CAN_BUS_STATE_ERROR_ACTIVE;
}

static can_err_t v_recover(Adapter* self, AdapterHandle h){
    (void)self; (void)h;
    // SocketCAN은 커널이 bus-off 복구 관리. 여기서는 NO-OP.
    return CAN_OK;
}

static void v_destroy(Adapter* self){
    if (!self) return;
    free(self->priv);
    free(self);
}

Adapter* adapter_linux_new(void){
    Adapter* ad = (Adapter*)calloc(1, sizeof(Adapter));
    if (!ad) return NULL;
    LinuxPriv* priv = (LinuxPriv*)calloc(1, sizeof(LinuxPriv));
    if (!priv){ free(ad); return NULL; }

    static const AdapterVTable V = {
        .probe            = v_probe,
        .ch_open          = v_ch_open,
        .ch_close         = v_ch_close,
        .ch_set_callbacks = v_ch_set_callbacks,
        .write            = v_write,
        .read             = v_read,
        .status           = v_status,
        .recover          = v_recover,
        .destroy          = v_destroy
    };
    ad->v = &V; ad->priv = priv;
    return ad;
}
