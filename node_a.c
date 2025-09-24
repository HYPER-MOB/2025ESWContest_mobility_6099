#include "node_a.h"
#include "can_api.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char* CH = "twai0";

// 유틸: 프레임 출력
static void print_frame(const char* tag, const CanFrame* f){
    printf("[%s] id=0x%X dlc=%u flags=0x%X data:",
           tag, (unsigned)f->id, (unsigned)f->dlc, (unsigned)f->flags);
    for (uint8_t i=0;i<f->dlc && i<8;i++) printf(" %02X", f->data[i]);
    printf("\n");
}

// 모든 수신 프레임 표시 (상대 노드의 에코 등)
static void on_rx(const CanFrame* f, void* user){
    (void)user;
    print_frame("RX-CB(A)", f);
}

// 전송 직전 data[2] 증가
static void seq_producer(CanFrame* io_frame, void* user){
    (void)user;
    static uint8_t seq = 0;
    if (io_frame->dlc < 3) io_frame->dlc = 3;
    io_frame->data[2] = seq++;
}

void node_a_setup(void){
    printf("[NodeA] setup\n");

    if (can_init(CAN_DEVICE_ESP32) != CAN_OK) {
        printf("can_init failed\n");
        return;
    }

    CanConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.channel     = 0;
    cfg.bitrate     = 500000;      // Node B와 동일
    cfg.samplePoint = 0.875f;
    cfg.sjw         = 1;
    cfg.mode        = CAN_MODE_NORMAL;

    if (can_open(CH, cfg) != CAN_OK) {
        printf("can_open failed\n");
        return;
    }
    printf("can_open OK (NORMAL)\n");

    // 모든 프레임 구독
    CanFilter any;
    memset(&any, 0, sizeof(any));
    any.type = CAN_FILTER_MASK;
    any.data.mask.id   = 0;
    any.data.mask.mask = 0;
    if (can_subscribe(CH, any, on_rx, NULL) <= 0) {
        printf("subscribe failed\n");
    }

    // job_ex 등록 (0x321, 500ms)
    CanFrame init;
    memset(&init, 0, sizeof(init));
    init.id   = 0x321;
    init.dlc  = 4;
    init.data[0]=0xAA; init.data[1]=0x55; init.data[2]=0x00; init.data[3]=0xEE;

    // 확장 버전: 전송 직전 seq_producer가 호출되어 data[2]가 매번 증가
    extern int can_register_job_ex(const char*, const CanFrame*, uint32_t, void (*)(CanFrame*, void*), void*);
    int jid = can_register_job_ex(CH, &init, 500, seq_producer, NULL);
    printf("[NodeA] job_ex registered: id=%d (0x%X every 500ms)\n", jid, (unsigned)init.id);

    printf("[NodeA] ready\n");
}

void node_a_loop(void){
    // 비차단 — 필요 시 상태 확인/추가 로직
}
