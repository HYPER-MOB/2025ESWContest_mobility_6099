// g++/gcc -std=c11 tests/router_canapi.c -o router_canapi -lpthread
// ����: sudo ./router_canapi
//
// ���� ���(�Ʒ� main.c�� can_api ��Ÿ�� �ؼ�)
// - CAN ID_REQ_BLE(0x100) ���� �� ./sca_ble_peripheral ����
//      - CAN DLC==8 �̸� �� 8����Ʈ�� 16-hex ���ڿ��� ��ȯ�Ͽ� --hash�� ���
//      - �ƴϸ� DEFAULT_HASH ���
//      - sca_ble_peripheral �����ڵ� 0 �� ID_RSP_BLE(0x110, 0x00) �۽�
//        ����/Ÿ�Ӿƿ� �� ID_RSP_BLE(0x110, 0xFF)
// - CAN ID_REQ_NFC(0x101) ���� �� ./test_nfc_log ���� �� UID �Ľ�
//      - EXPECTED_NFC_UID�� ��ġ�ϸ� ID_RSP_NFC(0x111, 0x00) �۽�
//        �ƴϸ� 0xFF
//
// �غ�
// - ���� ������ sca_ble_peripheral, test_nfc_log ���̳ʸ� ����
// - SocketCAN: sudo ip link set can0 up type can bitrate 500000
//
// ���� �� �ʿ�: can_api.h, canmessage.h, �׸��� �ش� ���̺귯������ main.c���� ���� �Ͱ� �����ϰ� ��ũ�Ǿ� �־�� ��.

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

/* ====== ���/ID ���� ====== */
static const uint32_t ID_REQ_BLE = 0x100;
static const uint32_t ID_REQ_NFC = 0x101;
static const uint32_t ID_RSP_BLE = 0x110;
static const uint32_t ID_RSP_NFC = 0x111;

/* ====== ���� ====== */
// BLE hash �⺻��(16-hex; 8����Ʈ �� "A1B2C3D4E5F60708")
static const char* DEFAULT_HASH = "A1B2C3D4E5F60708";
static const char* ADV_NAME = "SCA-CAR";
static const int   BLE_TIMEOUT = 60;

// NFC ��� UID(�빮��/�������)
static const char* EXPECTED_NFC_UID = "04AABBCCDDEEFF";

// ä�� �̸�(main.c�� ���� ��Ÿ��)
static const char* CH = "debug0";

/* ====== ��ƿ ====== */

// ����Ʈ �� 16�� ���ڿ�(�빮��)
static void bytes_to_hex(const uint8_t* d, size_t n, char* out16 /* �ּ� 2*n+1 */) {
    static const char* HEX = "0123456789ABCDEF";
    for (size_t i = 0; i < n; i++) {
        out16[i * 2 + 0] = HEX[(d[i] >> 4) & 0xF];
        out16[i * 2 + 1] = HEX[(d[i]) & 0xF];
    }
    out16[n * 2] = '\0';
}

// popen���� �ܺ� ���� & ǥ����� ���� ����
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

        // ������ ���� ����(�ʿ��� ��ŭ��)
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

// BLE �α׿��� data=... ����(������ ������ ���� ���)
static void parse_ble_written_data(char** lines, size_t n, char* out, size_t outsz) {
    if (!out || outsz == 0) return;
    out[0] = '\0';
    for (size_t i = 0; i < n; i++) {
        const char* p = strstr(lines[i], "data=");
        if (p) {
            p += 5;
            // �ٳ� ���� ����
            size_t L = strnlen(p, 1023);
            while (L > 0 && (p[L - 1] == '\n' || p[L - 1] == '\r')) L--;
            size_t cpy = (L < outsz - 1) ? L : (outsz - 1);
            memcpy(out, p, cpy); out[cpy] = '\0';
        }
    }
}

// NFC ��¿��� UID=... ����
static bool parse_uid_from_line(const char* s, char* uid, size_t uidsz) {
    const char* p = strstr(s, "UID");
    if (!p) return false;
    const char* q = strpbrk(p, "=:");
    if (!q) return false;
    q++;
    // 16���ڸ� ������(�빮��)
    size_t j = 0;
    for (; *q && j < uidsz - 1; q++) {
        if (isxdigit((unsigned char)*q)) {
            uid[j++] = (char)toupper((unsigned char)*q);
        }
    }
    uid[j] = '\0';
    return (j >= 6); // ���� �� ����Ʈ��
}

/* ====== ���� �۽� ====== */
// ���� ����Ʈ �ڵ� �۽�
static void send_rsp_byte(uint32_t canid, uint8_t code) {
    CanFrame fr; memset(&fr, 0, sizeof(fr));
    fr.id = canid; fr.dlc = 1; fr.data[0] = code;
    can_err_t r = can_send(CH, fr, 0);
    if (r != CAN_OK) {
        fprintf(stderr, "[router] send_rsp_byte(0x%X) error=%d\n", (unsigned)canid, r);
    }
}

/* ====== ��Ŀ ������: BLE ó�� ====== */
typedef struct {
    uint8_t data[8];
    uint8_t dlc;
} BleReq;

static void* ble_worker(void* arg) {
    BleReq req = *(BleReq*)arg; free(arg);

    // hash ����
    char hash16[33] = { 0 };
    if (req.dlc == 8) {
        bytes_to_hex(req.data, 8, hash16); // 16-hex
        printf("[router] use hash from CAN: %s\n", hash16);
    }
    else {
        strncpy(hash16, DEFAULT_HASH, sizeof(hash16) - 1);
        printf("[router] use default hash: %s\n", hash16);
    }

    // ����
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "./sca_ble_peripheral --hash %s --name %s --timeout %d",
        hash16, ADV_NAME, BLE_TIMEOUT);

    char** lines = NULL; size_t n = 0;
    int rc = run_and_capture_lines(cmd, "logs/ble_run.log", &lines, &n);

    char written[256]; parse_ble_written_data(lines, n, written, sizeof(written));
    printf("[router] BLE captured=\"%s\" exit=%d\n", written, rc);

    // ����: 0x00=����, 0xFF=����
    send_rsp_byte(ID_RSP_BLE, (rc == 0) ? 0x00 : 0xFF);

    free_lines(lines, n);
    return NULL;
}

/* ====== ��Ŀ ������: NFC ó�� ====== */
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

    // ����
    send_rsp_byte(ID_RSP_NFC, match ? 0x00 : 0xFF);

    free_lines(lines, n);
    return NULL;
}

/* ====== ���� �ݹ� ====== */
static void on_rx(const CanFrame* f, void* user) {
    (void)user;
    printf("[RX] id=0x%X dlc=%u :", (unsigned)f->id, f->dlc);
    for (uint8_t i = 0; i < f->dlc && i < 8; i++) printf(" %02X", f->data[i]);
    puts("");

    if (f->id == ID_REQ_BLE) {
        // ���ŷ ����: ���� ������� ó��
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

    // �� �� ID�� ����(�ʿ� �� Ȯ��)
}

/* ====== main (can_api �ʱ�ȭ/����) ====== */
int main(void) {
    // can_api �ʱ�ȭ
    if (can_init(CAN_DEVICE_DEBUG) != CAN_OK) {
        fprintf(stderr, "can_init failed\n");
        return 1;
    }

    // ä�� ����(main.c�� ���� ��Ÿ��)
    CanConfig cfg = { .channel = 0, .bitrate = 500000, .samplePoint = 0.875f, .sjw = 1, .mode = CAN_MODE_NORMAL };
    if (can_open(CH, cfg) != CAN_OK) {
        fprintf(stderr, "can_open failed\n");
        return 1;
    }

    // ����: 0x100, 0x101�� ����(�ʿ� �� �߰�)
    // can_api�� ����ũ ���Ͱ� ������ ���� �������� ������ ���� �ٸ� �� �־�, ���⼭�� "���" ���� �� on_rx���� ID �б��ص� OK
    CanFilter any = { .type = CAN_FILTER_MASK };
    any.data.mask.id = 0x000;
    any.data.mask.mask = 0x000; // ����
    if (can_subscribe(CH, any, on_rx, NULL) <= 0) {
        fprintf(stderr, "can_subscribe failed\n");
        return 1;
    }

    puts("[router_canapi] ready. Waiting CAN requests (BLE/NFC) ��");
    // ������ ���� ���: can_api ���� �����尡 �ݹ��� ȣ������ ��
    while (1) {
        msleep(500);
    }

    // �������� ����. ���� ��ƾ�� �ʿ��ϸ� �ñ׳� �ڵ鷯 �߰��ص� ��.
    can_dispose();
    return 0;
}
