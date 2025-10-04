// adapter_linux.c — Raspberry Pi / Jetson Nano (Linux, SocketCAN)
#include "adapter.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <poll.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#ifdef USE_LIBSOCKETCAN
  #include <libsocketcan.h>
  #include <linux/can/netlink.h>   // CAN_CTRLMODE_* 플래그
#endif

/* ========= Linux 전용 채널 핸들 ========= */
typedef struct Job {
    int id;
    CanFrame fr;                 // 내부 복사본
    uint32_t period_ms;
    uint64_t next_due_ms;        // 만기 시간
    can_tx_prepare_cb_t prep;    // 옵션 콜백
    void* prep_user;
    struct Job* next;
} Job;

typedef struct {
    int sock;                    // SocketCAN fd
    char ifname[IFNAMSIZ];

    // 콜백들 (adapter → channel)
    adapter_rx_cb_t  on_rx;   void* on_rx_user;
    adapter_err_cb_t on_err;  void* on_err_user;
    adapter_bus_cb_t on_bus;  void* on_bus_user;

    // RX 스레드
    pthread_t rx_thread;
    volatile int rx_running;

    // TX(Job) 스케줄러
    pthread_t tx_thread;
    volatile int tx_running;
    pthread_mutex_t mtx;      // job 리스트 보호
    Job* jobs;
    int  next_job_id;

} LinuxCh;

typedef struct { int dummy; } LinuxPriv;

/* ========= 유틸 ========= */
static inline uint64_t now_ms(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec*1000ULL + (uint64_t)ts.tv_nsec/1000000ULL;
}

static void canframe_from_linux(const struct can_frame* in, CanFrame* out){
    out->id    = in->can_id & CAN_EFF_MASK;
    out->flags = 0;
    if (in->can_id & CAN_EFF_FLAG) out->flags |= CAN_FRAME_EXTID;
    if (in->can_id & CAN_RTR_FLAG) out->flags |= CAN_FRAME_RTR;
    out->dlc = in->can_dlc;
    memset(out->data, 0, 8);
    if (!(out->flags & CAN_FRAME_RTR)) {
        memcpy(out->data, in->data, out->dlc);
    }
}

static void linux_from_canframe(const CanFrame* in, struct can_frame* out){
    memset(out, 0, sizeof(*out));
    canid_t cid = in->id & CAN_SFF_MASK;
    if (in->flags & CAN_FRAME_EXTID) {
        cid = (in->id & CAN_EFF_MASK) | CAN_EFF_FLAG;
    }
    if (in->flags & CAN_FRAME_RTR) {
        cid |= CAN_RTR_FLAG;
    }
    out->can_id  = cid;
    out->can_dlc = (in->dlc > 8) ? 8 : in->dlc;
    if (!(in->flags & CAN_FRAME_RTR)) {
        memcpy(out->data, in->data, out->can_dlc);
    }
}

#ifndef USE_LIBSOCKETCAN
/* === 옵션 2: libsocketcan 없을 때 fallback (원치 않으면 제거 가능) === */
static int fallback_bringup_with_ip(const char* ifname, int bitrate, int listen_only){
    char cmd[256];
    // down (있어도 되고 없어도 됨)
    snprintf(cmd, sizeof(cmd), "/sbin/ip link set %s down", ifname);
    (void)system(cmd);

    // bitrate 설정
    if (listen_only) {
        snprintf(cmd, sizeof(cmd),
                 "/sbin/ip link set %s type can bitrate %d dbitrate %d "
                 "fd off berr-reporting on", ifname, bitrate, bitrate);
        // listen-only는 ctrlmode에서 처리하는 게 정석이라 여기선 생략/주석
    } else {
        snprintf(cmd, sizeof(cmd),
                 "/sbin/ip link set %s type can bitrate %d", ifname, bitrate);
    }
    if (system(cmd) != 0) return -1;

    // up
    snprintf(cmd, sizeof(cmd), "/sbin/ip link set %s up", ifname);
    if (system(cmd) != 0) return -1;

    return 0;
}
#endif

static int bringup_can_iface(const char* ifname, const CanConfig* cfg){
    if (!ifname || !cfg) return -EINVAL;

#ifdef USE_LIBSOCKETCAN
    // 1) stop (down)
    int r = can_do_stop(ifname);
    if (r && r != -ENETDOWN) {
        fprintf(stderr, "can_do_stop(%s): %s\n", ifname, strerror(-r));
        return r;
    }

    // 2) bitrate 설정
    r = can_set_bitrate(ifname, cfg->bitrate);
    if (r) {
        fprintf(stderr, "can_set_bitrate(%s,%d): %s\n", ifname, cfg->bitrate, strerror(-r));
        return r;
    }

    // 3) ctrlmode (listen-only 등) 설정
    //    CAN_MODE_LOOPBACK / SILENT_LOOPBACK은 사용자 공간에서 완벽 대응이 애매하므로
    //    최소 listen-only만 반영. (loopback은 소켓 레벨 옵션으로도 가능)
    struct can_ctrlmode cm = {0};
    switch (cfg->mode) {
        case CAN_MODE_SILENT:
            cm.mask  = CAN_CTRLMODE_LISTENONLY;
            cm.flags = CAN_CTRLMODE_LISTENONLY;
            break;
        default:
            cm.mask  = CAN_CTRLMODE_LISTENONLY;
            cm.flags = 0;
            break;
    }
    if (cm.mask) {
        r = can_set_ctrlmode(ifname, &cm);
        if (r) {
            fprintf(stderr, "can_set_ctrlmode(%s): %s\n", ifname, strerror(-r));
            // 치명적이지 않으니 계속 진행할 수도 있음
        }
    }

    // (선택) 자동 재시작 ms
    // r = can_set_restart_ms(ifname, 100);

    // 4) start (up)
    r = can_do_start(ifname);
    if (r) {
        fprintf(stderr, "can_do_start(%s): %s\n", ifname, strerror(-r));
        return r;
    }
    return 0;

#else
    // libsocketcan 없음 → fallback (선택 사항)
    int listen_only = (cfg->mode == CAN_MODE_SILENT);
    int r = fallback_bringup_with_ip(ifname, cfg->bitrate, listen_only);
    if (r != 0) {
        fprintf(stderr, "fallback ip(%s) bring-up failed. Need root/CAP_NET_ADMIN?\n", ifname);
        return -EPERM;
    }
    return 0;
#endif
}

/* ========= RX 스레드 ========= */
static void* rx_thread_fn(void* arg){
    LinuxCh* ch = (LinuxCh*)arg;

    // 논블로킹 poll 루프
    struct pollfd pfd;
    pfd.fd = ch->sock;
    pfd.events = POLLIN;

    while (ch->rx_running){
        int r = poll(&pfd, 1, 100); // 100ms 타임아웃
        if (r > 0 && (pfd.revents & POLLIN)){
            struct can_frame fr;
            ssize_t n = read(ch->sock, &fr, sizeof(fr));
            if (n == (ssize_t)sizeof(fr)){
                CanFrame f; canframe_from_linux(&fr, &f);
                if (ch->on_rx) ch->on_rx(&f, ch->on_rx_user);
            }
        }
        // 에러/버스 상태 변화 감지도 여기서 가능(필요 시)
    }
    return NULL;
}

/* ========= TX(Job) 스케줄러 스레드 =========
 * - 락 안에서 오래 머무르지 않도록, 보낼 Job 포인터만 모아두고
 *   락 밖에서 prep + transmit 수행
 */
static void* tx_thread_fn(void* arg){
    LinuxCh* ch = (LinuxCh*)arg;
    while (ch->tx_running){
        uint64_t t = now_ms();
        uint32_t sleep_ms = 50;

        // 1) 락 잡고 이번 턴에 보낼 Job들을 수집
        //   (간단히 동적 배열 사용: jobs 개수만큼 재할당)
        Job** to_send = NULL;
        size_t ns = 0, cap = 0;

        pthread_mutex_lock(&ch->mtx);
        for (Job* j=ch->jobs; j; j=j->next){
            if (j->next_due_ms == 0) j->next_due_ms = t + j->period_ms;
            if (t >= j->next_due_ms){
                if (ns == cap){
                    size_t ncap = (cap==0)?8:cap*2;
                    Job** tmp = (Job**)realloc(to_send, ncap*sizeof(Job*));
                    if (!tmp) break; // 메모리 부족 시 이번 턴 일부만 전송
                    to_send = tmp; cap = ncap;
                }
                to_send[ns++] = j;

                // catch-up: backlog가 커도 수학적으로 점프
                uint64_t late = t - j->next_due_ms;
                uint64_t k = late / j->period_ms + 1;
                j->next_due_ms += k * j->period_ms;
            }
            uint32_t remain = (j->next_due_ms > t) ? (uint32_t)(j->next_due_ms - t) : 0;
            if (remain < sleep_ms) sleep_ms = remain;
        }
        pthread_mutex_unlock(&ch->mtx);

        // 2) 락 밖에서 전송
        for (size_t i=0;i<ns;i++){
            Job* j = to_send[i];
            CanFrame fr;

            // 프레임 스냅샷 (짧게 보호)
            pthread_mutex_lock(&ch->mtx);
            fr = j->fr;
            pthread_mutex_unlock(&ch->mtx);

            // 전송 직전 동적 갱신
            if (j->prep) j->prep(&fr, j->prep_user);

            struct can_frame lfr; linux_from_canframe(&fr, &lfr);
            // 논블록 송신 (소켓 버퍼 꽉 차면 drop되므로 필요시 poll로 POLLOUT 대기)
            (void)write(ch->sock, &lfr, sizeof(lfr));
        }

        if (to_send) free(to_send);

        // 3) 슬립
        struct timespec ts;
        ts.tv_sec = sleep_ms/1000;
        ts.tv_nsec = (sleep_ms%1000)*1000000L;
        nanosleep(&ts, NULL);
    }
    return NULL;
}

/* ========= VTable 구현 ========= */
static can_err_t v_probe(Adapter* self){
    (void)self;
    // 여기서 특별히 확인할 수 있는 건 없음(소켓 레벨). OK 반환.
    return CAN_OK;
}

static can_err_t v_ch_open(Adapter* self, const char* name, const CanConfig* cfg, AdapterHandle* out){
    if (!name || !cfg || !out) return CAN_ERR_INVALID;

    // (A) 먼저 인터페이스 bring-up 시도 (비트레이트/모드 반영)
    int br = bringup_can_iface(name, cfg);
    if (br) {
        // 권한 문제 or 존재하지 않는 인터페이스
        if (br == -EPERM || br == -EACCES) return CAN_ERR_PERMISSION;
        if (br == -ENODEV)                 return CAN_ERR_NODEV;
        // 그 외는 IO 오류로 보고
        // 계속 진행해도 bind가 될 수 있지만, 일관성 위해 실패로 처리
        return CAN_ERR_IO;
    }

    // (B) 소켓 생성 및 바인드
    int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) return CAN_ERR_IO;

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, name, IFNAMSIZ-1);

    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        close(s);
        return CAN_ERR_NODEV;
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    // (선택) 소켓 레벨 loopback/recv-own 설정 (내부 루프백 on/off)
    // int loopback = (cfg->mode == CAN_MODE_LOOPBACK || cfg->mode == CAN_MODE_SILENT_LOOPBACK) ? 1 : 0;
    // setsockopt(s, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof(loopback));

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(s);
        return CAN_ERR_IO;
    }

    // 논블록
    fcntl(s, F_SETFL, O_NONBLOCK);

    // 이하 기존 LinuxCh 할당, RX/TX 스레드 시작 로직 동일
    LinuxCh* ch = (LinuxCh*)calloc(1, sizeof(LinuxCh));
    if (!ch){ close(s); return CAN_ERR_MEMORY; }

    ch->sock = s;
    strncpy(ch->ifname, name, IFNAMSIZ-1);
    pthread_mutex_init(&ch->mtx, NULL);
    ch->jobs = NULL;
    ch->next_job_id = 0;

    ch->rx_running = 1;
    pthread_create(&ch->rx_thread, NULL, rx_thread_fn, ch);
    ch->tx_running = 1;
    pthread_create(&ch->tx_thread, NULL, tx_thread_fn, ch);

    *out = (AdapterHandle)ch;
    return CAN_OK;
}

static void v_ch_close(Adapter* self, AdapterHandle h){
    (void)self;
    if (!h) return;
    LinuxCh* ch = (LinuxCh*)h;

    // 스레드 종료
    ch->rx_running = 0;
    ch->tx_running = 0;

    // 가벼운 깨우기(논블록이라 자연 종료; 필요시 잠깐 delay)
    struct timespec ts = {.tv_sec=0,.tv_nsec=100*1000000L};
    nanosleep(&ts, NULL);

    pthread_join(ch->rx_thread, NULL);
    pthread_join(ch->tx_thread, NULL);

    // Job 정리
    pthread_mutex_lock(&ch->mtx);
    Job* j = ch->jobs;
    while (j){
        Job* nx = j->next;
        free(j);
        j = nx;
    }
    ch->jobs = NULL;
    pthread_mutex_unlock(&ch->mtx);

    pthread_mutex_destroy(&ch->mtx);

    if (ch->sock >= 0) close(ch->sock);
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
    ch->on_rx = on_rx;   ch->on_rx_user  = on_rx_user;
    ch->on_err= on_err;  ch->on_err_user = on_err_user;
    ch->on_bus= on_bus;  ch->on_bus_user = on_bus_user;
    return CAN_OK;
}

static can_err_t v_write(Adapter* self, AdapterHandle h, const CanFrame* fr, uint32_t timeout_ms){
    (void)self;
    if (!h || !fr) return CAN_ERR_INVALID;
    LinuxCh* ch = (LinuxCh*)h;

    struct can_frame lfr; linux_from_canframe(fr, &lfr);

    if (timeout_ms == 0){
        ssize_t n = write(ch->sock, &lfr, sizeof(lfr));
        if (n == (ssize_t)sizeof(lfr)) return CAN_OK;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return CAN_ERR_AGAIN;
        return CAN_ERR_IO;
    } else {
        struct pollfd pfd; pfd.fd = ch->sock; pfd.events = POLLOUT;
        int r = poll(&pfd, 1, (int)timeout_ms);
        if (r <= 0) return (r==0)?CAN_ERR_TIMEOUT:CAN_ERR_IO;
        ssize_t n = write(ch->sock, &lfr, sizeof(lfr));
        if (n == (ssize_t)sizeof(lfr)) return CAN_OK;
        return CAN_ERR_IO;
    }
}

static can_err_t v_read(Adapter* self, AdapterHandle h, CanFrame* out, uint32_t timeout_ms){
    (void)self;
    if (!h || !out) return CAN_ERR_INVALID;
    LinuxCh* ch = (LinuxCh*)h;

    struct pollfd pfd; pfd.fd = ch->sock; pfd.events = POLLIN;
    int r = poll(&pfd, 1, (int)timeout_ms);
    if (r <= 0) return (r==0)?CAN_ERR_TIMEOUT:CAN_ERR_IO;

    struct can_frame lfr;
    ssize_t n = read(ch->sock, &lfr, sizeof(lfr));
    if (n == (ssize_t)sizeof(lfr)){
        canframe_from_linux(&lfr, out);
        return CAN_OK;
    }
    return CAN_ERR_IO;
}

static can_bus_state_t v_status(Adapter* self, AdapterHandle h){
    (void)self; (void)h;
    // SocketCAN에서 사용자 공간에서 버스오프 직접 판단은 제한적.
    // 간단히 ACTIVE로 보고, 에러프레임/통계 확장도 가능.
    return CAN_BUS_STATE_ERROR_ACTIVE;
}

static can_err_t v_recover(Adapter* self, AdapterHandle h){
    (void)self; (void)h;
    // 사용자 공간에서 bus-off recover는 일반적으로 드라이버/if 재시작 필요.
    // 여기서는 NOT SUPPORTED.
    return CAN_ERR_STATE;
}

/* ====== Job 등록/취소/확장 ====== */
static can_err_t v_ch_register_job_ex(Adapter* self, int* id, AdapterHandle h, const CanFrame* initial, uint32_t period_ms, can_tx_prepare_cb_t prep, void* prep_user)
{
    (void)self;
    if (!h || !initial || period_ms==0) return CAN_ERR_INVALID;
    LinuxCh* ch = (LinuxCh*)h;

    Job* j = (Job*)calloc(1, sizeof(Job));
    if (!j) return CAN_ERR_INVALID;
    j->fr = *initial;
    j->period_ms = period_ms;
    j->next_due_ms = 0;
    j->prep = prep;
    j->prep_user = prep_user;

    pthread_mutex_lock(&ch->mtx);
    j->id = ++ch->next_job_id;
    j->next = ch->jobs;
    ch->jobs = j;
    pthread_mutex_unlock(&ch->mtx);

    *id = j->id;
    
    return CAN_OK;
}

static can_err_t v_ch_register_job(Adapter* self, int* id, AdapterHandle h, const CanFrame* fr, uint32_t period_ms)
{
    return v_ch_register_job_ex(self, id, h, fr, period_ms, NULL, NULL);
}

static can_err_t v_ch_cancel_job(Adapter* self, AdapterHandle h, int jobId){
    (void)self;
    if (!h || jobId<=0) return CAN_ERR_INVALID;
    LinuxCh* ch = (LinuxCh*)h;

    can_err_t ret = CAN_ERR_INVALID;
    pthread_mutex_lock(&ch->mtx);
    Job** pp = &ch->jobs;
    while (*pp){
        if ((*pp)->id == jobId){
            Job* del = *pp; *pp = del->next; free(del);
            ret = CAN_OK; break;
        }
        pp = &(*pp)->next;
    }
    pthread_mutex_unlock(&ch->mtx);
    return ret;
}

static void v_destroy(Adapter* self){
    if (!self) return;
    free(self->priv);
    free(self);
}

/* ====== 팩토리 ====== */
Adapter* adapter_linux_new(void){
    Adapter* ad = (Adapter*)calloc(1, sizeof(Adapter));
    if (!ad) return NULL;
    LinuxPriv* priv = (LinuxPriv*)calloc(1, sizeof(LinuxPriv));
    if (!priv){ free(ad); return NULL; }

    static const AdapterVTable V = {
        .probe              = v_probe,
        .ch_open            = v_ch_open,
        .ch_close           = v_ch_close,
        .ch_set_callbacks   = v_ch_set_callbacks,
        .write              = v_write,
        .read               = v_read,
        .status             = v_status,
        .recover            = v_recover,
        .ch_register_job    = v_ch_register_job,
        .ch_register_job_ex = v_ch_register_job_ex,
        .ch_cancel_job      = v_ch_cancel_job,
        .destroy            = v_destroy
    };
    ad->v = &V; ad->priv = priv;
    return ad;
}

Adapter* adapter_esp32_new(void) { return NULL; }