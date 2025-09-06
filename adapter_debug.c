// adapter_debug_win.c — Windows 전용(Win32) 스레드 기반 디버그 어댑터
// 하드웨어 CAN 없이 메모리 큐로 송수신/콜백 테스트
// 빌드: MSVC(cl) 또는 MinGW-w64(gcc)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#ifndef WINVER
#define WINVER 0x0600
#endif
#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "adapter.h"   // can_api.h 포함됨

// -------------------- 내부 자료구조 --------------------

typedef struct {
    CanFrame   q[256];
    unsigned   head, tail, size;

    CRITICAL_SECTION cs;
    CONDITION_VARIABLE cv;

    // 콜백
    adapter_rx_cb_t  on_rx;
    void*            on_rx_user;
    adapter_err_cb_t on_err;
    void*            on_err_user;
    adapter_bus_cb_t on_bus;
    void*            on_bus_user;

    // 워커 스레드
    HANDLE      worker;
    volatile LONG running;  // 0/1

    // 상태
    char        name[16];
    int         open;
} DebugCh;

typedef struct {
    // 프로세스 단위 공용 상태가 필요하면 여기에
    int dummy;
} DebugPriv;

// -------------------- 유틸 --------------------

static inline bool is_full(const DebugCh* ch)  { return ch->size == 256; }
static inline bool is_empty(const DebugCh* ch) { return ch->size == 0; }

static void enqueue(DebugCh* ch, const CanFrame* fr){
    ch->q[ch->tail] = *fr;
    ch->tail = (ch->tail + 1u) % 256u;
    ch->size++;
}

static void dequeue(DebugCh* ch, CanFrame* out){
    *out = ch->q[ch->head];
    ch->head = (ch->head + 1u) % 256u;
    ch->size--;
}

// 백그라운드 워커: 큐에서 프레임을 꺼내 on_rx 호출
static DWORD WINAPI rx_worker_thread(LPVOID param){
    DebugCh* ch = (DebugCh*)param;

    EnterCriticalSection(&ch->cs);
    while (InterlockedCompareExchange(&ch->running, 0, 0) == 1) {
        while (is_empty(ch) && InterlockedCompareExchange(&ch->running, 0, 0) == 1) {
            SleepConditionVariableCS(&ch->cv, &ch->cs, INFINITE);
        }
        if (!ch->open || InterlockedCompareExchange(&ch->running, 0, 0) != 1) break;

        // 한 개 꺼내서 콜백
        CanFrame f;
        if (!is_empty(ch)) {
            dequeue(ch, &f);

            adapter_rx_cb_t cb = ch->on_rx;
            void* u = ch->on_rx_user;

            // 콜백 동안 잠금 해제 (re-entrancy 방지)
            LeaveCriticalSection(&ch->cs);
            if (cb) cb(&f, u);
            EnterCriticalSection(&ch->cs);
        }
    }
    LeaveCriticalSection(&ch->cs);
    return 0;
}

// -------------------- VTable 구현 --------------------

static can_err_t v_probe(Adapter* self) {
    (void)self;
    // 디버그 어댑터는 항상 사용 가능하다고 가정
    return CAN_OK;
}

static can_err_t v_ch_open(Adapter* self, const char* name, const CanConfig* cfg, AdapterHandle* out_handle) {
    (void)self; (void)cfg;
    if (!out_handle) return CAN_ERR_INVALID;

    DebugCh* ch = (DebugCh*)calloc(1, sizeof(DebugCh));
    if (!ch) return CAN_ERR_MEMORY;

    InitializeCriticalSection(&ch->cs);
    InitializeConditionVariable(&ch->cv);
    ch->open = 1;
    if (name) {
        strncpy(ch->name, name, sizeof(ch->name)-1);
        ch->name[sizeof(ch->name)-1] = 0;
    }

    // 워커 스레드 시작 (on_rx 콜백이 등록되어 있어도/없어도 일관되게 큐를 소비)
    InterlockedExchange(&ch->running, 1);
    ch->worker = CreateThread(NULL, 0, rx_worker_thread, ch, 0, NULL);
    if (!ch->worker) {
        DeleteCriticalSection(&ch->cs);
        free(ch);
        return CAN_ERR_IO;
    }

    *out_handle = (AdapterHandle)ch;
    return CAN_OK;
}

static void v_ch_close(Adapter* self, AdapterHandle handle) {
    (void)self;
    if (!handle) return;
    DebugCh* ch = (DebugCh*)handle;

    EnterCriticalSection(&ch->cs);
    ch->open = 0;
    InterlockedExchange(&ch->running, 0);
    WakeAllConditionVariable(&ch->cv);
    LeaveCriticalSection(&ch->cs);

    if (ch->worker) {
        WaitForSingleObject(ch->worker, INFINITE);
        CloseHandle(ch->worker);
        ch->worker = NULL;
    }
    DeleteCriticalSection(&ch->cs);
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
    DebugCh* ch = (DebugCh*)h;

    EnterCriticalSection(&ch->cs);
    ch->on_rx       = on_rx;
    ch->on_rx_user  = on_rx_user;
    ch->on_err      = on_err;
    ch->on_err_user = on_err_user;
    ch->on_bus      = on_bus;
    ch->on_bus_user = on_bus_user;
    LeaveCriticalSection(&ch->cs);

    return CAN_OK;
}

// write: 큐에 넣고 워커를 깨워서 on_rx까지 이어지게 함
static can_err_t v_write(Adapter* self, AdapterHandle handle, const CanFrame* fr, uint32_t timeout_ms) {
    (void)self; (void)timeout_ms;
    if (!handle || !fr) return CAN_ERR_INVALID;
    DebugCh* ch = (DebugCh*)handle;

    EnterCriticalSection(&ch->cs);
    if (!ch->open) { LeaveCriticalSection(&ch->cs); return CAN_ERR_STATE; }
    if (is_full(ch)) { LeaveCriticalSection(&ch->cs); return CAN_ERR_AGAIN; }

    enqueue(ch, fr);
    WakeConditionVariable(&ch->cv);
    LeaveCriticalSection(&ch->cs);
    return CAN_OK;
}

// read: 큐에서 직접 꺼냄 (콜백 워커가 이미 소비했을 수 있음에 유의)
static can_err_t v_read(Adapter* self, AdapterHandle handle, CanFrame* out, uint32_t timeout_ms) {
    (void)self;
    if (!handle || !out) return CAN_ERR_INVALID;
    DebugCh* ch = (DebugCh*)handle;

    EnterCriticalSection(&ch->cs);
    if (!ch->open) { LeaveCriticalSection(&ch->cs); return CAN_ERR_STATE; }

    if (timeout_ms == 0) {
        if (is_empty(ch)) { LeaveCriticalSection(&ch->cs); return CAN_ERR_AGAIN; }
        dequeue(ch, out);
        LeaveCriticalSection(&ch->cs);
        return CAN_OK;
    }

    DWORD wait_ms = (timeout_ms == UINT32_MAX) ? INFINITE : timeout_ms;
    while (is_empty(ch)) {
        BOOL ok = SleepConditionVariableCS(&ch->cv, &ch->cs, wait_ms);
        if (!ok) {
            // 타임아웃 또는 기타 실패
            if (GetLastError() == ERROR_TIMEOUT) { LeaveCriticalSection(&ch->cs); return CAN_ERR_TIMEOUT; }
            LeaveCriticalSection(&ch->cs); return CAN_ERR_IO;
        }
        if (!ch->open) { LeaveCriticalSection(&ch->cs); return CAN_ERR_STATE; }
        // 깨어나도 여전히 empty일 수 있으니 루프 계속
    }
    dequeue(ch, out);
    LeaveCriticalSection(&ch->cs);
    return CAN_OK;
}

static can_bus_state_t v_status(Adapter* self, AdapterHandle handle) {
    (void)self; (void)handle;
    return CAN_BUS_STATE_ERROR_ACTIVE; // 항상 정상 가정
}

static can_err_t v_recover(Adapter* self, AdapterHandle handle) {
    (void)self; (void)handle;
    return CAN_OK;
}

static void v_destroy(Adapter* self) {
    if (!self) return;
    if (self->priv) free(self->priv);
    free(self);
}

Adapter* adapter_debug_new(void) {
    Adapter* ad = (Adapter*)calloc(1, sizeof(Adapter));
    if (!ad) return NULL;
    DebugPriv* priv = (DebugPriv*)calloc(1, sizeof(DebugPriv));
    if (!priv) { free(ad); return NULL; }

    static const AdapterVTable V = {
        .probe           = v_probe,
        .ch_open         = v_ch_open,
        .ch_close        = v_ch_close,
        .ch_set_callbacks= v_ch_set_callbacks,
        .write           = v_write,
        .read            = v_read,
        .status          = v_status,
        .recover         = v_recover,
        .destroy         = v_destroy
    };

    ad->v    = &V;
    ad->priv = priv;
    return ad;
}