#include "node_b.h"
#include "can_api.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char* CH = "twai0";

static void print_frame(const char* tag, const CanFrame* f){
    printf("[%s] id=0x%X dlc=%u flags=0x%X data:",
           tag, (unsigned)f->id, (unsigned)f->dlc, (unsigned)f->flags);
    for (uint8_t i=0;i<f->dlc && i<8;i++) printf(" %02X", f->data[i]);
    printf("\n");
}

// 0x321 수신 시 즉시 0x322로 에코
static void on_rx(const CanFrame* f, void* user){
    (void)user;
    print_frame("RX-CB(B)", f);
    if (f->id == 0x321) {
        CanFrame rsp = *f;
        rsp.id = 0x322;
        if (rsp.dlc < 3) rsp.dlc = 3;
        rsp.data[2] = (uint8_t)(rsp.data[2] + 1);
        if (can_send(CH, rsp, 0) == CAN_OK) {
            print_frame("TX-ECHO", &rsp);
        }
    }
}

// Heartbeat: 전송 직전 data[0] 증가
static void hb_producer(CanFrame* io_frame, void* user){
    (void)user;
    static uint8_t hb = 0;
    if (io_frame->dlc < 1) io_frame->dlc = 1;
    io_frame->data[0] = hb++;
}

void node_b_setup(void){
    printf("[NodeB] setup\n");

    if (can_init(CAN_DEVICE_ESP32) != CAN_OK) {
        printf("can_init failed\n");
        return;
    }

    CanConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.channel     = 0;
    cfg.bitrate     = 500000;    // Node A와 동일
    cfg.samplePoint = 0.875f;
    cfg.sjw         = 1;
    cfg.mode        = CAN_MODE_NORMAL;

    if (can_open(CH, cfg) != CAN_OK) {
        printf("can_open failed\n");
        return;
    }
    printf("can_open OK (NORMAL)\n");

    // 모든 프레임 구독 (에코용)
    CanFilter any;
    memset(&any, 0, sizeof(any));
    any.type = CAN_FILTER_MASK;
    any.data.mask.id   = 0;
    any.data.mask.mask = 0;
    if (can_subscribe(CH, any, on_rx, NULL) <= 0) {
        printf("subscribe failed\n");
    }

    // Heartbeat job_ex (0x700, 1000ms)
    CanFrame hb;
    memset(&hb, 0, sizeof(hb));
    hb.id  = 0x700;
    hb.dlc = 1;
    hb.data[0] = 0x00;

    extern int can_register_job_ex(const char*, const CanFrame*, uint32_t, void (*)(CanFrame*, void*), void*);
    int jid_hb = can_register_job_ex(CH, &hb, 1000, hb_producer, NULL);
    printf("[NodeB] heartbeat job_ex registered: id=%d (0x%X every 1000ms)\n", jid_hb, (unsigned)hb.id);

    printf("[NodeB] ready\n");
}

void node_b_loop(void){
    // 비차단 — 필요 시 상태 확인/추가 로직
}
