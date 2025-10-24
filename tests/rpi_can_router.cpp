// g++/gcc -std=c11 tests/router_canapi.c -o router_canapi -lpthread
// 실행: sudo ./router_canapi
//
// 동작 요약(아래 main.c의 can_api 스타일 준수)
// - CAN ID_REQ_BLE(0x100) 수신 → ./sca_ble_peripheral 실행
//      - CAN DLC==8 이면 그 8바이트를 16-hex 문자열로 변환하여 --hash에 사용
//      - 아니면 DEFAULT_HASH 사용
//      - sca_ble_peripheral 종료코드 0 → ID_RSP_BLE(0x110, 0x00) 송신
//        실패/타임아웃 → ID_RSP_BLE(0x110, 0xFF)
// - CAN ID_REQ_NFC(0x101) 수신 → ./test_nfc_log 실행 → UID 파싱
//      - EXPECTED_NFC_UID와 일치하면 ID_RSP_NFC(0x111, 0x00) 송신
//        아니면 0xFF
//
// 준비물
// - 같은 폴더에 sca_ble_peripheral, test_nfc_log 바이너리 존재
// - SocketCAN: sudo ip link set can0 up type can bitrate 500000
//
// 빌드 시 필요: can_api.h, canmessage.h, 그리고 해당 라이브러리들이 main.c에서 쓰는 것과 동일하게 링크되어 있어야 함.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <pthread.h>

#include "can_api.h"
#include "canmessage.h"

#ifdef _WIN32
#include <windows.h>
static void msleep(unsigned ms) { Sleep(ms); }
#else
#include <unistd.h>
static void msleep(unsigned ms) { usleep(ms * 1000); }
#endif

/* ====== 상수/ID 정의 ====== */
static const uint32_t ID_REQ_BLE = 0x100;
static const uint32_t ID_REQ_NFC = 0x101;
static const uint32_t ID_RSP_BLE = 0x110;
static const uint32_t ID_RSP_NFC = 0x111;

/* ====== 설정 ====== */
// BLE hash 기본값(16-hex; 8바이트 → "A1B2C3D4E5F60708")
static const char* DEFAULT_HASH = "A1B2C3D4E5F60708";
static const char* ADV_NAME = "SCA-CAR";
static const int   BLE_TIMEOUT = 60;

// NFC 기대 UID(대문자/공백없음)
static const char* EXPECTED_NFC_UID = "04AABBCCDDEEFF";

// 채널 이름(main.c와 동일 스타일)
static const char* CH = "debug0";

/* ====== 유틸 ====== */

// 바이트 → 16진 문자열(대문자)
static void bytes_to_hex(const uint8_t* d, size_t n, char* out16 /* 최소 2*n+1 */) {
    static const char* HEX = "0123456789ABCDEF";
    for (size_t i = 0; i < n; i++) {
        out16[i * 2 + 0] = HEX[(d[i] >> 4) & 0xF];
        out16[i * 2 + 1] = HEX[(d[i]) & 0xF];
    }
    out16[n * 2] = '\0';
}

// popen으로 외부 실행 & 표준출력 라인 수집
static int run_and_capture_lines(const char* cmd, const char* logfile,
    char*** out_lines, size_t* out_count) {
    system("mkdir -p logs");
    FILE* fp = popen(cmd, "r");
    FILE* lg = fopen(logfile, "a");
    if (!fp) {
        if (lg) { fprintf(lg, "[router] popen failed: %s\n", cmd); fclose(lg); }
        fprintf(stderr, "[router] popen failed: %s\n", cmd);
        return 1;
    }

    char** lines = NULL;
    size_t count = 0;
    char buf[1024];

    fprintf(stdout, "[router] spawn: %s\n", cmd);
    if (lg) fprintf(lg, "[router] spawn: %s\n", cmd);

    while (fgets(buf, sizeof(buf), fp)) {
        if (lg) fputs(buf, lg);
        fputs(buf, stdout);

        // 적당히 라인 저장(필요한 만큼만)
        char* s = strdup(buf);
        if (!s) continue;
        char** nl = (char**)realloc(lines, sizeof(char*) * (count + 1));
        if (!nl) { free(s); continue; }
        lines = nl; lines[count++] = s;
    }
    int status = pclose(fp);
    int exitcode = (WIFEXITED(status) ? WEXITSTATUS(status) : 1);

    if (lg) {
        fprintf(lg, "[router] proc exit=%d\n", exitcode);
        fclose(lg);
    }
    *out_lines = lines; *out_count = count;
    return exitcode;
}

static void free_lines(char** lines, size_t n) {
    for (size_t i = 0; i < n; i++) free(lines[i]);
    free(lines);
}

// BLE 로그에서 data=... 추출(있으면 마지막 값을 기록)
static void parse_ble_written_data(char** lines, size_t n, char* out, size_t outsz) {
    if (!out || outsz == 0) return;
    out[0] = '\0';
    for (size_t i = 0; i < n; i++) {
        const char* p = strstr(lines[i], "data=");
        if (p) {
            p += 5;
            // 줄끝 개행 제거
            size_t L = strnlen(p, 1023);
            while (L > 0 && (p[L - 1] == '\n' || p[L - 1] == '\r')) L--;
            size_t cpy = (L < outsz - 1) ? L : (outsz - 1);
            memcpy(out, p, cpy); out[cpy] = '\0';
        }
    }
}

// NFC 출력에서 UID=... 추출
static bool parse_uid_from_line(const char* s, char* uid, size_t uidsz) {
    const char* p = strstr(s, "UID");
    if (!p) return false;
    const char* q = strpbrk(p, "=:");
    if (!q) return false;
    q++;
    // 16진자만 모으기(대문자)
    size_t j = 0;
    for (; *q && j < uidsz - 1; q++) {
        if (isxdigit((unsigned char)*q)) {
            uid[j++] = (char)toupper((unsigned char)*q);
        }
    }
    uid[j] = '\0';
    return (j >= 6); // 대충 몇 바이트라도
}

/* ====== 응답 송신 ====== */
// 단일 바이트 코드 송신
static void send_rsp_byte(uint32_t canid, uint8_t code) {
    CanFrame fr; memset(&fr, 0, sizeof(fr));
    fr.id = canid; fr.dlc = 1; fr.data[0] = code;
    can_err_t r = can_send(CH, fr, 0);
    if (r != CAN_OK) {
        fprintf(stderr, "[router] send_rsp_byte(0x%X) error=%d\n", (unsigned)canid, r);
    }
}

/* ====== 워커 스레드: BLE 처리 ====== */
typedef struct {
    uint8_t data[8];
    uint8_t dlc;
} BleReq;

static void* ble_worker(void* arg) {
    BleReq req = *(BleReq*)arg; free(arg);

    // hash 결정
    char hash16[33] = { 0 };
    if (req.dlc == 8) {
        bytes_to_hex(req.data, 8, hash16); // 16-hex
        printf("[router] use hash from CAN: %s\n", hash16);
    }
    else {
        strncpy(hash16, DEFAULT_HASH, sizeof(hash16) - 1);
        printf("[router] use default hash: %s\n", hash16);
    }

    // 실행
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "./sca_ble_peripheral --hash %s --name %s --timeout %d",
        hash16, ADV_NAME, BLE_TIMEOUT);

    char** lines = NULL; size_t n = 0;
    int rc = run_and_capture_lines(cmd, "logs/ble_run.log", &lines, &n);

    char written[256]; parse_ble_written_data(lines, n, written, sizeof(written));
    printf("[router] BLE captured=\"%s\" exit=%d\n", written, rc);

    // 응답: 0x00=성공, 0xFF=실패
    send_rsp_byte(ID_RSP_BLE, (rc == 0) ? 0x00 : 0xFF);

    free_lines(lines, n);
    return NULL;
}

/* ====== 워커 스레드: NFC 처리 ====== */
static void* nfc_worker(void* arg) {
    (void)arg;

    char** lines = NULL; size_t n = 0;
    int rc = run_and_capture_lines("./test_nfc_log", "logs/nfc_run.log", &lines, &n);

    char uid[256] = { 0 };
    for (size_t i = 0; i < n; i++) {
        if (parse_uid_from_line(lines[i], uid, sizeof(uid))) break;
    }
    bool match = (uid[0] && strcmp(uid, EXPECTED_NFC_UID) == 0);

    printf("[router] NFC uid=\"%s\" expected=\"%s\" match=%s exit=%d\n",
        uid, EXPECTED_NFC_UID, match ? "true" : "false", rc);

    // 응답
    send_rsp_byte(ID_RSP_NFC, match ? 0x00 : 0xFF);

    free_lines(lines, n);
    return NULL;
}

/* ====== 수신 콜백 ====== */
static void on_rx(const CanFrame* f, void* user) {
    (void)user;
    printf("[RX] id=0x%X dlc=%u :", (unsigned)f->id, f->dlc);
    for (uint8_t i = 0; i < f->dlc && i < 8; i++) printf(" %02X", f->data[i]);
    puts("");

    if (f->id == ID_REQ_BLE) {
        // 블로킹 방지: 별도 스레드로 처리
        BleReq* req = (BleReq*)malloc(sizeof(BleReq));
        if (!req) return;
        memset(req, 0, sizeof(*req));
        req->dlc = f->dlc;
        memcpy(req->data, f->data, (f->dlc <= 8 ? f->dlc : 8));
        pthread_t th; pthread_create(&th, NULL, ble_worker, req);
        pthread_detach(th);
    }
    else if (f->id == ID_REQ_NFC) {
        pthread_t th; pthread_create(&th, NULL, nfc_worker, NULL);
        pthread_detach(th);
    }

    // 그 외 ID는 무시(필요 시 확장)
}

/* ====== main (can_api 초기화/구독) ====== */
int main(void) {
    // can_api 초기화
    if (can_init(CAN_DEVICE_DEBUG) != CAN_OK) {
        fprintf(stderr, "can_init failed\n");
        return 1;
    }

    // 채널 열기(main.c와 동일 스타일)
    CanConfig cfg = { .channel = 0, .bitrate = 500000, .samplePoint = 0.875f, .sjw = 1, .mode = CAN_MODE_NORMAL };
    if (can_open(CH, cfg) != CAN_OK) {
        fprintf(stderr, "can_open failed\n");
        return 1;
    }

    // 필터: 0x100, 0x101만 구독(필요 시 추가)
    // can_api의 마스크 필터가 프레임 단위 전역인지 구현에 따라 다를 수 있어, 여기서는 "모두" 구독 후 on_rx에서 ID 분기해도 OK
    CanFilter any = { .type = CAN_FILTER_MASK };
    any.data.mask.id = 0x000;
    any.data.mask.mask = 0x000; // 전부
    if (can_subscribe(CH, any, on_rx, NULL) <= 0) {
        fprintf(stderr, "can_subscribe failed\n");
        return 1;
    }

    puts("[router_canapi] ready. Waiting CAN requests (BLE/NFC) …");
    // 간단히 영속 대기: can_api 내부 스레드가 콜백을 호출해줄 것
    while (1) {
        msleep(500);
    }

    // 도달하지 않음. 정리 루틴이 필요하면 시그널 핸들러 추가해도 됨.
    can_dispose();
    return 0;
}
