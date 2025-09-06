// main.c : CAN Debug CLI (frame + message + periodic jobs)
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "can_api.h"
#include "canmessage.h"

#ifdef _WIN32
  #include <windows.h>
  static void msleep(unsigned ms){ Sleep(ms); }
#else
  #include <unistd.h>
  static void msleep(unsigned ms){ usleep(ms*1000); }
#endif

/* ---------- Job Tracker ---------- */
typedef struct {
    int       job_id;     // can_register_job 반환 값
    CanFrame* fr;         // 주기 전송할 프레임(영속 할당)
    uint32_t  period_ms;  // 정보 표시용
} JobItem;

#define MAX_JOBS 64
static JobItem g_jobs[MAX_JOBS];

static void jobs_init(void){
    memset(g_jobs, 0, sizeof(g_jobs));
}
static int jobs_put(int job_id, CanFrame* fr, uint32_t period_ms){
    for (int i=0;i<MAX_JOBS;i++){
        if (g_jobs[i].job_id == 0){
            g_jobs[i].job_id = job_id;
            g_jobs[i].fr = fr;
            g_jobs[i].period_ms = period_ms;
            return 1;
        }
    }
    return 0;
}
static JobItem* jobs_find(int job_id){
    for (int i=0;i<MAX_JOBS;i++) if (g_jobs[i].job_id == job_id) return &g_jobs[i];
    return NULL;
}
static void jobs_remove(int job_id){
    for (int i=0;i<MAX_JOBS;i++){
        if (g_jobs[i].job_id == job_id){
            if (g_jobs[i].fr) free(g_jobs[i].fr);
            g_jobs[i].job_id = 0;
            g_jobs[i].fr = NULL;
            g_jobs[i].period_ms = 0;
            return;
        }
    }
}
static void jobs_clear_and_stop_all(const char* chname){
    for (int i=0;i<MAX_JOBS;i++){
        if (g_jobs[i].job_id){
            can_cancel_job(chname, g_jobs[i].job_id);
            if (g_jobs[i].fr) free(g_jobs[i].fr);
            g_jobs[i].job_id = 0;
            g_jobs[i].fr = NULL;
        }
    }
}

/* ---------- 유틸 ---------- */
static void print_frame(const char* tag, const CanFrame* f){
    printf("[%s] id=0x%X dlc=%u flags=0x%X data:", tag, (unsigned)f->id, f->dlc, (unsigned)f->flags);
    for (uint8_t i=0;i<f->dlc && i<8;i++) printf(" %02X", f->data[i]);
    puts("");
}
static int parse_hex_byte(const char* s, uint8_t* out){
    char* end = NULL;
    unsigned long v = strtoul(s, &end, 16);
    if (end==s || v>0xFF) return 0;
    *out = (uint8_t)v;
    return 1;
}
static int parse_id(const char* s, uint32_t* out){
    char* end=NULL;
    int base = (strlen(s)>2 && (s[0]=='0') && (s[1]=='x' || s[1]=='X')) ? 16 : 0;
    unsigned long v = strtoul(s, &end, base);
    if (end==s || v>0x1FFFFFFF) return 0;
    *out = (uint32_t)v;
    return 1;
}

/* ---------- 수신 콜백(프레임 + 디코드 출력) ---------- */
static void print_decoded(can_msg_id_t id, const CanMessage* m){
    switch (id){
    case BCAN_ID_DCU_SEAT_ORDER:
        printf("  [DECODE] DCU_SEAT_ORDER: pos=%u angle=%u front=%u rear=%u\n",
            m->dcu_seat_order.sig_seat_position,
            m->dcu_seat_order.sig_seat_angle,
            m->dcu_seat_order.sig_seat_front_height,
            m->dcu_seat_order.sig_seat_rear_height);
        break;
    case BCAN_ID_DCU_MIRROR_ORDER:
        printf("  [DECODE] DCU_MIRROR_ORDER: L(yaw=%u,pitch=%u) R(yaw=%u,pitch=%u) ROOM(yaw=%u,pitch=%u)\n",
            m->dcu_mirror_order.sig_mirror_left_yaw,
            m->dcu_mirror_order.sig_mirror_left_pitch,
            m->dcu_mirror_order.sig_mirror_right_yaw,
            m->dcu_mirror_order.sig_mirror_right_pitch,
            m->dcu_mirror_order.sig_mirror_room_yaw,
            m->dcu_mirror_order.sig_mirror_room_pitch);
        break;
    case BCAN_ID_POW_SEAT_STATE:
        printf("  [DECODE] POW_SEAT_STATE: pos=%u angle=%u front=%u rear=%u flag=%u\n",
            m->pow_seat_state.sig_seat_position,
            m->pow_seat_state.sig_seat_angle,
            m->pow_seat_state.sig_seat_front_height,
            m->pow_seat_state.sig_seat_rear_height,
            m->pow_seat_state.sig_seat_flag);
        break;
    default:
        puts("  [DECODE] (unknown id)");
        break;
    }
}
static void on_rx(const CanFrame* f, void* user){
    (void)user;
    print_frame("RX", f);
    CanMessage m;
    can_msg_id_t id = can_decode(f, &m);
    print_decoded(id, &m);
}

/* ---------- 도움말 ---------- */
static void show_help(void){
    puts("Commands:");
    puts("  send <id> <bytes...>            e.g., send 0x123 01 02 03 04 | send 456 AABB");
    puts("  sendmsg seat_order <pos> <angle> <front> <rear>");
    puts("  sendmsg mirror_order <Lyaw> <Lpitch> <Ryaw> <Rpitch> <RoomYaw> <RoomPitch>");
    puts("  sendmsg seat_state  <pos> <angle> <front> <rear> <flag>");
    puts("  job add <period_ms> <id> <bytes...>");
    puts("  job addmsg <period_ms> seat_order <pos> <angle> <front> <rear>");
    puts("  job addmsg <period_ms> mirror_order <Lyaw> <Lpitch> <Ryaw> <Rpitch> <RoomYaw> <RoomPitch>");
    puts("  job addmsg <period_ms> seat_state  <pos> <angle> <front> <rear> <flag>");
    puts("  job cancel <jobId>");
    puts("  job list");
    puts("  recv [timeout_ms]");
    puts("  sleep <ms>");
    puts("  help");
    puts("  quit / exit");
}

/* ---------- main ---------- */
int main(void){
    const char* CH = "debug0";

    jobs_init();

    if (can_init(CAN_DEVICE_DEBUG) != CAN_OK){
        fprintf(stderr, "can_init failed\n");
        return 1;
    }
    CanConfig cfg = { .channel=0, .bitrate=500000, .samplePoint=0.875f, .sjw=1, .mode=CAN_MODE_NORMAL };
    if (can_open(CH, cfg) != CAN_OK){
        fprintf(stderr, "can_open failed\n");
        return 1;
    }

    // 모든 프레임 구독
    CanFilter any = { .type = CAN_FILTER_MASK };
    any.data.mask.id = 0; any.data.mask.mask = 0;
    if (can_subscribe(CH, any, on_rx, NULL) <= 0){
        fprintf(stderr, "can_subscribe failed\n");
        return 1;
    }

    char line[512];
    puts("CAN Debug CLI (frame + message + jobs). Type 'help'.");

    while (1){
        printf("> "); fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        size_t n = strlen(line);
        if (n && (line[n-1]=='\n' || line[n-1]=='\r')) line[--n]='\0';

        // 토큰화
        char* tok[64]; int ntok=0;
        char* p=line;
        while (*p && ntok < (int)(sizeof(tok)/sizeof(tok[0]))){
            while (*p && isspace((unsigned char)*p)) ++p;
            if (!*p) break;
            tok[ntok++] = p;
            while (*p && !isspace((unsigned char)*p)) ++p;
            if (*p){ *p='\0'; ++p; }
        }
        if (ntok==0) continue;

        /* ----- 명령 분기 ----- */
        if (!strcmp(tok[0], "quit") || !strcmp(tok[0], "exit")){
            break;

        } else if (!strcmp(tok[0], "help")){
            show_help();

        } else if (!strcmp(tok[0], "sleep") && ntok>=2){
            unsigned ms = (unsigned)strtoul(tok[1], NULL, 10); msleep(ms);

        } else if (!strcmp(tok[0], "recv")){
            uint32_t to_ms = (ntok>=2) ? (uint32_t)strtoul(tok[1], NULL, 10) : 0;
            CanFrame rf = {0};
            can_err_t r = can_recv(CH, &rf, to_ms);
            if (r==CAN_OK){
                print_frame("RECV-POLL", &rf);
                CanMessage m; can_msg_id_t id = can_decode(&rf, &m);
                print_decoded(id, &m);
            } else if (r==CAN_ERR_TIMEOUT || r==CAN_ERR_AGAIN){
                puts("(no frame)");
            } else {
                fprintf(stderr, "recv error=%d\n", r);
            }

        } else if (!strcmp(tok[0], "send")){
            if (ntok < 2){ puts("usage: send <id> <hex bytes...>"); continue; }
            uint32_t id=0;
            if (!parse_id(tok[1], &id)){ puts("invalid id"); continue; }
            CanFrame tx = {0}; tx.id = id;
            int bi=0;
            for (int i=2; i<ntok && bi<8; ++i){
                const char* s = tok[i];
                size_t L = strlen(s);
                if (L>2){
                    if (L%2!=0){ puts("hex length must be even"); bi=0; break; }
                    for (size_t k=0; k<L && bi<8; k+=2){
                        char buf[3]={s[k], s[k+1], 0};
                        if (!parse_hex_byte(buf, &tx.data[bi++])){ puts("invalid hex"); bi=0; break; }
                    }
                } else {
                    if (!parse_hex_byte(s, &tx.data[bi++])){ puts("invalid hex"); bi=0; break; }
                }
            }
            tx.dlc = (uint8_t)bi;
            can_err_t r = can_send(CH, tx, 0);
            if (r!=CAN_OK) fprintf(stderr, "send error=%d\n", r);

        } else if (!strcmp(tok[0], "sendmsg")){
            if (ntok < 2){ puts("usage: sendmsg <type> ..."); continue; }
            CanMessage m; memset(&m, 0, sizeof(m));
            can_msg_id_t id = 0; uint8_t dlc = 0;

            if (!strcmp(tok[1], "seat_order")){
                if (ntok < 6){ puts("usage: sendmsg seat_order <pos> <angle> <front> <rear>"); continue; }
                m.dcu_seat_order.sig_seat_position     = (uint8_t)strtoul(tok[2], NULL, 0);
                m.dcu_seat_order.sig_seat_angle        = (uint8_t)strtoul(tok[3], NULL, 0);
                m.dcu_seat_order.sig_seat_front_height = (uint8_t)strtoul(tok[4], NULL, 0);
                m.dcu_seat_order.sig_seat_rear_height  = (uint8_t)strtoul(tok[5], NULL, 0);
                id = BCAN_ID_DCU_SEAT_ORDER; dlc = 4;

            } else if (!strcmp(tok[1], "mirror_order")){
                if (ntok < 8){ puts("usage: sendmsg mirror_order <Lyaw> <Lpitch> <Ryaw> <Rpitch> <RoomYaw> <RoomPitch>"); continue; }
                m.dcu_mirror_order.sig_mirror_left_yaw    = (uint8_t)strtoul(tok[2], NULL, 0);
                m.dcu_mirror_order.sig_mirror_left_pitch  = (uint8_t)strtoul(tok[3], NULL, 0);
                m.dcu_mirror_order.sig_mirror_right_yaw   = (uint8_t)strtoul(tok[4], NULL, 0);
                m.dcu_mirror_order.sig_mirror_right_pitch = (uint8_t)strtoul(tok[5], NULL, 0);
                m.dcu_mirror_order.sig_mirror_room_yaw    = (uint8_t)strtoul(tok[6], NULL, 0);
                m.dcu_mirror_order.sig_mirror_room_pitch  = (uint8_t)strtoul(tok[7], NULL, 0);
                id = BCAN_ID_DCU_MIRROR_ORDER; dlc = 6;

            } else if (!strcmp(tok[1], "seat_state")){
                if (ntok < 7){ puts("usage: sendmsg seat_state <pos> <angle> <front> <rear> <flag>"); continue; }
                m.pow_seat_state.sig_seat_position     = (uint8_t)strtoul(tok[2], NULL, 0);
                m.pow_seat_state.sig_seat_angle        = (uint8_t)strtoul(tok[3], NULL, 0);
                m.pow_seat_state.sig_seat_front_height = (uint8_t)strtoul(tok[4], NULL, 0);
                m.pow_seat_state.sig_seat_rear_height  = (uint8_t)strtoul(tok[5], NULL, 0);
                m.pow_seat_state.sig_seat_flag         = (uint8_t)strtoul(tok[6], NULL, 0);
                id = BCAN_ID_POW_SEAT_STATE; dlc = 5;

            } else {
                puts("unknown message type. supported: seat_order, mirror_order, seat_state");
                continue;
            }

            CanFrame fr = can_encode(id, &m, dlc);
            can_err_t r = can_send(CH, fr, 0);
            if (r!=CAN_OK) fprintf(stderr, "sendmsg error=%d\n", r);

        } else if (!strcmp(tok[0], "job")){
            if (ntok < 2){ puts("usage: job <add|addmsg|cancel|list> ..."); continue; }

            if (!strcmp(tok[1], "list")){
                puts("Active jobs:");
                for (int i=0;i<MAX_JOBS;i++){
                    if (g_jobs[i].job_id){
                        printf("  id=%d period=%u id=0x%X dlc=%u\n",
                            g_jobs[i].job_id, g_jobs[i].period_ms,
                            (unsigned)g_jobs[i].fr->id, g_jobs[i].fr->dlc);
                    }
                }
                continue;
            }

            if (!strcmp(tok[1], "cancel")){
                if (ntok < 3){ puts("usage: job cancel <jobId>"); continue; }
                int jid = (int)strtol(tok[2], NULL, 10);
                if (jid <= 0){ puts("invalid jobId"); continue; }
                can_err_t r = can_cancel_job(CH, jid);
                if (r == CAN_OK){
                    jobs_remove(jid);
                    printf("job %d canceled\n", jid);
                } else {
                    fprintf(stderr, "cancel failed err=%d\n", r);
                }
                continue;
            }

            if (!strcmp(tok[1], "add")){
                if (ntok < 4){ puts("usage: job add <period_ms> <id> <hex bytes...>"); continue; }
                uint32_t period = (uint32_t)strtoul(tok[2], NULL, 10);
                uint32_t id=0; if (!parse_id(tok[3], &id)){ puts("invalid id"); continue; }

                CanFrame* fr = (CanFrame*)calloc(1, sizeof(CanFrame));
                if (!fr){ puts("oom"); continue; }
                fr->id = id;
                int bi=0;
                for (int i=4; i<ntok && bi<8; ++i){
                    const char* s = tok[i];
                    size_t L = strlen(s);
                    if (L>2){
                        if (L%2!=0){ puts("hex length must be even"); bi=0; break; }
                        for (size_t k=0; k<L && bi<8; k+=2){
                            char buf[3]={s[k], s[k+1], 0};
                            if (!parse_hex_byte(buf, &fr->data[bi++])){ puts("invalid hex"); bi=0; break; }
                        }
                    } else {
                        if (!parse_hex_byte(s, &fr->data[bi++])){ puts("invalid hex"); bi=0; break; }
                    }
                }
                fr->dlc = (uint8_t)bi;

                int jid = can_register_job(CH, fr, period);
                if (jid <= 0){
                    fprintf(stderr, "register_job failed\n");
                    free(fr);
                } else {
                    if (!jobs_put(jid, fr, period)){
                        // 로컬 트래커 실패 시라도 잡은 건 취소해 주자
                        can_cancel_job(CH, jid);
                        free(fr);
                        fprintf(stderr, "job table full\n");
                    } else {
                        printf("job added: id=%d every %u ms (frame id=0x%X dlc=%u)\n",
                               jid, period, (unsigned)fr->id, fr->dlc);
                    }
                }
                continue;
            }

            if (!strcmp(tok[1], "addmsg")){
                if (ntok < 4){ puts("usage: job addmsg <period_ms> <type> ..."); continue; }
                uint32_t period = (uint32_t)strtoul(tok[2], NULL, 10);
                CanMessage m; memset(&m, 0, sizeof(m));
                can_msg_id_t id=0; uint8_t dlc=0;

                if (!strcmp(tok[3], "seat_order")){
                    if (ntok < 8){ puts("usage: job addmsg <period_ms> seat_order <pos> <angle> <front> <rear>"); continue; }
                    m.dcu_seat_order.sig_seat_position     = (uint8_t)strtoul(tok[4], NULL, 0);
                    m.dcu_seat_order.sig_seat_angle        = (uint8_t)strtoul(tok[5], NULL, 0);
                    m.dcu_seat_order.sig_seat_front_height = (uint8_t)strtoul(tok[6], NULL, 0);
                    m.dcu_seat_order.sig_seat_rear_height  = (uint8_t)strtoul(tok[7], NULL, 0);
                    id = BCAN_ID_DCU_SEAT_ORDER; dlc = 4;

                } else if (!strcmp(tok[3], "mirror_order")){
                    if (ntok < 10){ puts("usage: job addmsg <period_ms> mirror_order <Lyaw> <Lpitch> <Ryaw> <Rpitch> <RoomYaw> <RoomPitch>"); continue; }
                    m.dcu_mirror_order.sig_mirror_left_yaw    = (uint8_t)strtoul(tok[4], NULL, 0);
                    m.dcu_mirror_order.sig_mirror_left_pitch  = (uint8_t)strtoul(tok[5], NULL, 0);
                    m.dcu_mirror_order.sig_mirror_right_yaw   = (uint8_t)strtoul(tok[6], NULL, 0);
                    m.dcu_mirror_order.sig_mirror_right_pitch = (uint8_t)strtoul(tok[7], NULL, 0);
                    m.dcu_mirror_order.sig_mirror_room_yaw    = (uint8_t)strtoul(tok[8], NULL, 0);
                    m.dcu_mirror_order.sig_mirror_room_pitch  = (uint8_t)strtoul(tok[9], NULL, 0);
                    id = BCAN_ID_DCU_MIRROR_ORDER; dlc = 6;

                } else if (!strcmp(tok[3], "seat_state")){
                    if (ntok < 9){ puts("usage: job addmsg <period_ms> seat_state <pos> <angle> <front> <rear> <flag>"); continue; }
                    m.pow_seat_state.sig_seat_position     = (uint8_t)strtoul(tok[4], NULL, 0);
                    m.pow_seat_state.sig_seat_angle        = (uint8_t)strtoul(tok[5], NULL, 0);
                    m.pow_seat_state.sig_seat_front_height = (uint8_t)strtoul(tok[6], NULL, 0);
                    m.pow_seat_state.sig_seat_rear_height  = (uint8_t)strtoul(tok[7], NULL, 0);
                    m.pow_seat_state.sig_seat_flag         = (uint8_t)strtoul(tok[8], NULL, 0);
                    id = BCAN_ID_POW_SEAT_STATE; dlc = 5;

                } else {
                    puts("unknown message type. supported: seat_order, mirror_order, seat_state");
                    continue;
                }

                CanFrame fr = can_encode(id, &m, dlc);
                CanFrame* heap = (CanFrame*)malloc(sizeof(CanFrame));
                if (!heap){ puts("oom"); continue; }
                *heap = fr;

                int jid = can_register_job(CH, heap, period);
                if (jid <= 0){
                    fprintf(stderr, "register_job failed\n");
                    free(heap);
                } else {
                    if (!jobs_put(jid, heap, period)){
                        can_cancel_job(CH, jid);
                        free(heap);
                        fprintf(stderr, "job table full\n");
                    } else {
                        printf("job added: id=%d every %u ms (msg %u)\n", jid, period, (unsigned)id);
                    }
                }
                continue;
            }

            // 알 수 없는 job 서브커맨드
            puts("usage: job <add|addmsg|cancel|list> ...");

        } else {
            puts("unknown command. type 'help'.");
        }
    }

    // 종료 시 모든 job 정리
    jobs_clear_and_stop_all(CH);
    can_dispose();
    return 0;
}
