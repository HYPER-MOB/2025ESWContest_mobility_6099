// adapter_esp32.c (ESP-IDF)
// CMakeLists.txt에 idf_component_register(SRCS "adapter_esp32.c" ...)
// sdkconfig에서 TWAI 드라이버 사용 가능해야 함

#include "adapter.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include "driver/twai.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"

// ★ 환경에 맞게 핀 번호를 수정하세요
#ifndef TWAI_TX_PIN
#define TWAI_TX_PIN  5
#endif
#ifndef TWAI_RX_PIN
#define TWAI_RX_PIN  4
#endif

typedef struct Job {
    int id;
    CanFrame    fr;          // 프레임은 내부에 "복사" 저장
    uint32_t    period_ms;
    uint64_t    next_due_ms; // ms
    can_tx_prepare_cb_t prep;
    void*       prep_user;
    struct Job* next;
} Job;

typedef struct {
    CanFrame fr;                  // 프레임 스냅샷
    can_tx_prepare_cb_t prep;     // 콜백 스냅샷
    void* prep_user;
} Pending;

typedef struct {
    // TWAI에는 "채널" 하나 가정
    adapter_rx_cb_t  on_rx;  void* on_rx_user;
    adapter_err_cb_t on_err; void* on_err_user;
    adapter_bus_cb_t on_bus; void* on_bus_user;

    TaskHandle_t     rx_task;
    volatile int     running;

    TaskHandle_t      tx_task;
    volatile int      tx_running;
    int               next_job_id;
    Job*              jobs;
    SemaphoreHandle_t mtx;         // jobs 보호
} Esp32Ch;

typedef struct { int dummy; } Esp32Priv;

static inline uint64_t now_ms(void){
    return (uint64_t)(esp_timer_get_time() / 1000ULL);
}

// bitrate 매핑(대표값만; 필요시 더 추가)
static twai_timing_config_t timing_from_bitrate(int bitrate){
    twai_timing_config_t t;
    switch (bitrate){
        case 125000:  t = (twai_timing_config_t)TWAI_TIMING_CONFIG_125KBITS(); break;
        case 250000:  t = (twai_timing_config_t)TWAI_TIMING_CONFIG_250KBITS(); break;
        case 500000:  t = (twai_timing_config_t)TWAI_TIMING_CONFIG_500KBITS(); break;
        case 1000000: t = (twai_timing_config_t)TWAI_TIMING_CONFIG_1MBITS();   break;
        default:      t = (twai_timing_config_t)TWAI_TIMING_CONFIG_500KBITS(); break;
    }
    return t;
}

static void canframe_from_twai(const twai_message_t* in, CanFrame* out){
    out->id   = (in->extd) ? in->identifier & 0x1FFFFFFF : in->identifier & 0x7FF;
    out->dlc  = in->data_length_code;
    out->flags= 0;
    if (in->extd) out->flags |= CAN_FRAME_EXTID;
    if (in->rtr)  out->flags |= CAN_FRAME_RTR;
    // (에러 프레임 개념은 TWAI 메시지로 직접 안 들어옴)
    memset(out->data, 0, 8);
    if (!in->rtr) memcpy(out->data, in->data, out->dlc);
}

static void twai_from_canframe(const CanFrame* in, twai_message_t* out){
    memset(out, 0, sizeof(*out));
    out->identifier = in->id;
    out->extd = (in->flags & CAN_FRAME_EXTID) ? 1 : 0;
    out->rtr  = (in->flags & CAN_FRAME_RTR) ? 1 : 0;
    out->data_length_code = (in->dlc > 8) ? 8 : in->dlc;
    if (!out->rtr) memcpy(out->data, in->data, out->data_length_code);
}

static void rx_task_fn(void* arg){
    Esp32Ch* ch = (Esp32Ch*)arg;
    for(;;){
        // 종료 노티가 들어왔으면 바로 종료
        if (ulTaskNotifyTake(pdTRUE, 0) > 0) break;
        if (!ch->running) break;

        twai_message_t msg;
        if (twai_receive(&msg, pdMS_TO_TICKS(100)) == ESP_OK){
            CanFrame f; canframe_from_twai(&msg, &f);

            // 콜백/유저 포인터 스냅샷 후 호출
            adapter_rx_cb_t cb = ch->on_rx;
            void* u = ch->on_rx_user;
            if (cb) cb(&f, u);
        }
        // (선택) TWAI Alerts로 on_bus/on_err도 처리 가능
    }
    ch->rx_task = NULL;           // 핸들 무효화
    vTaskDelete(NULL);
}

static void tx_task_fn(void* arg){
    Esp32Ch* ch = (Esp32Ch*)arg;
    for(;;){
        // 종료 노티가 들어왔으면 바로 종료
        if (ulTaskNotifyTake(pdTRUE, 0) > 0) break;
        if (!ch->tx_running) break;

        uint64_t t = now_ms();
        uint32_t sleep_ms = 50;

        Pending pend[16]; int np = 0;
        bool overflow = false;

        // 1) 락 안에서 '무엇을 보낼지' 결정 + 스냅샷
        xSemaphoreTake(ch->mtx, portMAX_DELAY);
        for (Job* j = ch->jobs; j; j = j->next) {
            if (j->next_due_ms == 0) j->next_due_ms = t + j->period_ms; // or t;
            if (t >= j->next_due_ms) {
                if (np < (int)(sizeof(pend)/sizeof(pend[0]))) {
                    pend[np].fr        = j->fr;
                    pend[np].prep      = j->prep;
                    pend[np].prep_user = j->prep_user;
                    np++;
                } else {
                    overflow = true;   // 남은 건 다음 턴에
                }
                uint64_t late = t - j->next_due_ms;
                uint64_t k = late / j->period_ms + 1;
                j->next_due_ms += k * j->period_ms;
            }
            uint32_t remain = (j->next_due_ms > t) ? (uint32_t)(j->next_due_ms - t) : 0;
            if (remain < sleep_ms) sleep_ms = remain;
        }
        xSemaphoreGive(ch->mtx);

        if (overflow) sleep_ms = 0;     // 용량 초과 시 곧바로 다음 루프

        // 2) 락 밖에서 prep + 전송
        for (int i=0; i<np; ++i) {
            if (pend[i].prep) pend[i].prep(&pend[i].fr, pend[i].prep_user);
            twai_message_t m; twai_from_canframe(&pend[i].fr, &m);
            (void)twai_transmit(&m, 0); // 필요하면 결과 체크하여 통계/에러콜백
        }

        vTaskDelay(pdMS_TO_TICKS(sleep_ms ? sleep_ms : 1));
    }
    ch->tx_task = NULL;           // 핸들 무효화
    vTaskDelete(NULL);
}
/* ========== VTABLE ========== */

static can_err_t v_probe(Adapter* self){
    (void)self;
    // 실제 하드웨어 체크는 제한적. 여기선 OK.
    return CAN_OK;
}

static can_err_t v_ch_open(Adapter* self, const char* name, const CanConfig* cfg, AdapterHandle* out){
    (void)name;
    if (!cfg || !out) return CAN_ERR_INVALID;

    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(TWAI_TX_PIN, TWAI_RX_PIN, TWAI_MODE_NORMAL);
    // Listen-only/Loopback 등 모드 설정
    switch (cfg->mode){
        case CAN_MODE_SILENT:          g.mode = TWAI_MODE_LISTEN_ONLY; break;
        case CAN_MODE_LOOPBACK:        g.mode = TWAI_MODE_NO_ACK; /* 근사치 */ break;
        case CAN_MODE_SILENT_LOOPBACK: g.mode = TWAI_MODE_NO_ACK; /* 근사치 */ break;
        case CAN_MODE_NORMAL:
        default:                       g.mode = TWAI_MODE_NORMAL; break;
    }
    twai_timing_config_t t = timing_from_bitrate(cfg->bitrate);
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g, &t, &f) != ESP_OK) return CAN_ERR_IO;
    if (twai_start() != ESP_OK){ twai_driver_uninstall(); return CAN_ERR_IO; }

    Esp32Ch* ch = (Esp32Ch*)calloc(1, sizeof(Esp32Ch));
    if (!ch){ twai_stop(); twai_driver_uninstall(); return CAN_ERR_MEMORY; }

    ch->mtx = xSemaphoreCreateMutex();
    if (!ch->mtx){
        free(ch); twai_stop(); twai_driver_uninstall();
        return CAN_ERR_MEMORY;
    }

    ch->running = 1;
    if (xTaskCreate(rx_task_fn, "twai_rx", 4096, ch, 10, &ch->rx_task) != pdPASS){
        vSemaphoreDelete(ch->mtx);
        free(ch); twai_stop(); twai_driver_uninstall();
        return CAN_ERR_MEMORY;
    }


    ch->tx_running = 1;
    if (xTaskCreate(tx_task_fn, "twai_tx", 4096, ch, 9, &ch->tx_task) != pdPASS){
        // RX 태스크 종료 후 정리
        ch->running = 0; if (ch->rx_task) xTaskNotifyGive(ch->rx_task);
        for (int i=0; i<100; ++i){
            eTaskState st = ch->rx_task ? eTaskGetState(ch->rx_task) : eDeleted;
            if (st == eDeleted || !ch->rx_task) break;
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        vSemaphoreDelete(ch->mtx);
        free(ch); twai_stop(); twai_driver_uninstall();
        return CAN_ERR_MEMORY;
    }

    ch->next_job_id= 0;
    ch->jobs       = NULL;

    *out = (AdapterHandle)ch;
    return CAN_OK;
}

static void v_ch_close(Adapter* self, AdapterHandle h){
    (void)self;
    if (!h) return;
    Esp32Ch* ch = (Esp32Ch*)h;

    ch->running    = 0;
    ch->tx_running = 0;
    if (ch->rx_task) xTaskNotifyGive(ch->rx_task);
    if (ch->tx_task) xTaskNotifyGive(ch->tx_task);

    // 2) 태스크 종료 대기 (최대 ~1s)
    const int max_wait_iters = 100; // 100 * 10ms = 1s
    for (int i=0; i<max_wait_iters; ++i) {
        eTaskState rxst = ch->rx_task ? eTaskGetState(ch->rx_task) : eDeleted;
        eTaskState txst = ch->tx_task ? eTaskGetState(ch->tx_task) : eDeleted;
        if ((rxst == eDeleted || ch->rx_task == NULL) &&
            (txst == eDeleted || ch->tx_task == NULL)) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // 3) 잡/뮤텍스 정리 (이 시점에 태스크는 멈춰 있어야 안전)
    if (ch->mtx) xSemaphoreTake(ch->mtx, portMAX_DELAY);
    for (Job* j = ch->jobs; j; ){
        Job* nx = j->next; free(j); j = nx;
    }
    ch->jobs = NULL;
    if (ch->mtx){ xSemaphoreGive(ch->mtx); vSemaphoreDelete(ch->mtx); ch->mtx=NULL; }

    // 4) 드라이버 정지/해제
    twai_stop();
    twai_driver_uninstall();

    // 5) 마지막으로 컨트롤 블록 해제
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
    Esp32Ch* ch = (Esp32Ch*)h;
    ch->on_rx = on_rx; ch->on_rx_user = on_rx_user;
    ch->on_err = on_err; ch->on_err_user = on_err_user;
    ch->on_bus = on_bus; ch->on_bus_user = on_bus_user;
    return CAN_OK;
}

static can_err_t v_write(Adapter* self, AdapterHandle h, const CanFrame* fr, uint32_t timeout_ms){
    (void)self;
    if (!h || !fr) return CAN_ERR_INVALID;
    twai_message_t msg; twai_from_canframe(fr, &msg);
    TickType_t to = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    esp_err_t e = twai_transmit(&msg, to);
    if (e == ESP_OK) return CAN_OK;
    if (e == ESP_ERR_TIMEOUT) return CAN_ERR_TIMEOUT;
    return CAN_ERR_IO;
}

static can_err_t v_read(Adapter* self, AdapterHandle h, CanFrame* out, uint32_t timeout_ms){
    (void)self;
    if (!h || !out) return CAN_ERR_INVALID;
    twai_message_t msg;
    TickType_t to = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    esp_err_t e = twai_receive(&msg, to);
    if (e == ESP_OK){ canframe_from_twai(&msg, out); return CAN_OK; }
    if (e == ESP_ERR_TIMEOUT) return CAN_ERR_TIMEOUT;
    return CAN_ERR_IO;
}

static can_bus_state_t v_status(Adapter* self, AdapterHandle h){
    (void)self; (void)h;
    twai_status_info_t st;
    if (twai_get_status_info(&st) != ESP_OK) return CAN_BUS_STATE_ERROR_ACTIVE;
    if (st.state == TWAI_STATE_BUS_OFF) return CAN_BUS_STATE_BUS_OFF;
    if (st.state == TWAI_STATE_RECOVERING || st.rx_error_counter > 127 || st.tx_error_counter > 127)
        return CAN_BUS_STATE_ERROR_PASSIVE;
    return CAN_BUS_STATE_ERROR_ACTIVE;
}

static can_err_t v_recover(Adapter* self, AdapterHandle h){
    (void)self; (void)h;
    // 버스오프 시
    if (twai_initiate_recovery() == ESP_OK) return CAN_OK;
    return CAN_ERR_STATE;
}

static void v_destroy(Adapter* self){
    if (!self) return;
    free(self->priv);
    free(self);
}

static can_err_t v_ch_register_job_dynamic(Adapter* self, int* id, AdapterHandle h, can_tx_prepare_cb_t prep, void* prep_user, uint32_t period_ms)
{
    (void)self;
    if (!h || period_ms==0) return CAN_ERR_INVALID;
    Esp32Ch* ch = (Esp32Ch*)h;

    Job* j = (Job*)calloc(1, sizeof(Job));
    if (!j) return CAN_ERR_MEMORY;

    memset(&j->fr, 0, sizeof(j->fr));
    j->period_ms = period_ms;
    j->next_due_ms = now_ms() + period_ms;
    j->prep = prep;
    j->prep_user = prep_user;

    xSemaphoreTake(ch->mtx, portMAX_DELAY);
    j->id = ++ch->next_job_id;
    j->next = ch->jobs;
    ch->jobs = j;
    xSemaphoreGive(ch->mtx);

    *id = j->id;

    return CAN_OK;
}

static can_err_t v_ch_register_job(Adapter* self, int* id, AdapterHandle h, const CanFrame* fr, uint32_t period_ms){
    (void)self;
    if (!h || !fr || period_ms==0) return CAN_ERR_INVALID;
    Esp32Ch* ch = (Esp32Ch*)h;

    Job* j = (Job*)calloc(1, sizeof(Job));
    if (!j) return CAN_ERR_MEMORY;

    j->fr = *fr;
    j->period_ms = period_ms;
    j->next_due_ms = now_ms() + period_ms;
    j->prep = NULL;
    j->prep_user = NULL;

    xSemaphoreTake(ch->mtx, portMAX_DELAY);
    j->id = ++ch->next_job_id;
    j->next = ch->jobs;
    ch->jobs = j;
    xSemaphoreGive(ch->mtx);

    *id = j->id;

    return CAN_OK;
}

static can_err_t v_ch_cancel_job(Adapter* self, AdapterHandle h, int jobId){
    (void)self;
    if (!h || jobId<=0) return CAN_ERR_INVALID;
    Esp32Ch* ch = (Esp32Ch*)h;

    can_err_t ret = CAN_ERR_INVALID;
    xSemaphoreTake(ch->mtx, portMAX_DELAY);
    Job** pp = &ch->jobs;
    while (*pp){
        if ((*pp)->id == jobId){
            Job* del = *pp; *pp = del->next; free(del);
            ret = CAN_OK; break;
        }
        pp = &(*pp)->next;
    }
    xSemaphoreGive(ch->mtx);
    return ret;
}

Adapter* adapter_esp32_new(void){
    Adapter* ad = (Adapter*)calloc(1, sizeof(Adapter));
    if (!ad) return NULL;
    Esp32Priv* priv = (Esp32Priv*)calloc(1, sizeof(Esp32Priv));
    if (!priv){ free(ad); return NULL; }

    static const AdapterVTable V = {
        .probe                      = v_probe,
        .ch_open                    = v_ch_open,
        .ch_close                   = v_ch_close,
        .ch_set_callbacks           = v_ch_set_callbacks,
        .write                      = v_write,
        .read                       = v_read,
        .status                     = v_status,
        .recover                    = v_recover,
        .ch_register_job            = v_ch_register_job,
        .ch_register_job_dynamic    = v_ch_register_job_dynamic,
        .ch_cancel_job              = v_ch_cancel_job,   
        .destroy                    = v_destroy
    };
    ad->v = &V; ad->priv = priv;
    return ad;
}

Adapter* adapter_linux_new(void) { return NULL; }