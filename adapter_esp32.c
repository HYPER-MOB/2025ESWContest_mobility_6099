// adapter_esp32.c (ESP-IDF)
// CMakeLists.txt에 idf_component_register(SRCS "adapter_esp32.c" ...)
// sdkconfig에서 TWAI 드라이버 사용 가능해야 함

#include "adapter.h"
#include <string.h>
#include "driver/twai.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ★ 환경에 맞게 핀 번호를 수정하세요
#ifndef TWAI_TX_PIN
#define TWAI_TX_PIN  5
#endif
#ifndef TWAI_RX_PIN
#define TWAI_RX_PIN  4
#endif

typedef struct {
    // TWAI에는 "채널" 하나 가정
    adapter_rx_cb_t  on_rx;  void* on_rx_user;
    adapter_err_cb_t on_err; void* on_err_user;
    adapter_bus_cb_t on_bus; void* on_bus_user;

    TaskHandle_t     rx_task;
    volatile int     running;
} Esp32Ch;

typedef struct { int dummy; } Esp32Priv;

// bitrate 매핑(대표값만; 필요시 더 추가)
static twai_timing_config_t timing_from_bitrate(int bitrate){
    switch (bitrate){
        case 125000: return TWAI_TIMING_CONFIG_125KBITS();
        case 250000: return TWAI_TIMING_CONFIG_250KBITS();
        case 500000: return TWAI_TIMING_CONFIG_500KBITS();
        case 1000000:return TWAI_TIMING_CONFIG_1MBITS();
        default:     return TWAI_TIMING_CONFIG_500KBITS();
    }
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
    while (ch->running){
        twai_message_t msg;
        // 타임아웃 100ms
        if (twai_receive(&msg, pdMS_TO_TICKS(100)) == ESP_OK){
            CanFrame f; canframe_from_twai(&msg, &f);
            if (ch->on_rx) ch->on_rx(&f, ch->on_rx_user);
        }
        // (선택) 상태 변화 감지 시 on_bus/on_err 호출 가능
    }
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
    ch->running = 1;
    xTaskCreate(rx_task_fn, "twai_rx", 4096, ch, 10, &ch->rx_task);

    *out = (AdapterHandle)ch;
    return CAN_OK;
}

static void v_ch_close(Adapter* self, AdapterHandle h){
    (void)self;
    if (!h) return;
    Esp32Ch* ch = (Esp32Ch*)h;
    ch->running = 0;
    // rx_task는 loop 끝에서 자가 삭제됨
    twai_stop();
    twai_driver_uninstall();
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

Adapter* adapter_esp32_new(void){
    Adapter* ad = (Adapter*)calloc(1, sizeof(Adapter));
    if (!ad) return NULL;
    Esp32Priv* priv = (Esp32Priv*)calloc(1, sizeof(Esp32Priv));
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
