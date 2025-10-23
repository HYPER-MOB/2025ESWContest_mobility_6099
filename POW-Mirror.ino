#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "can_api.h"
#include "canmessage.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// CAN ========================================================
static const char* CH = "twai0";
static const int NODE_IDX = 2;
static QueueHandle_t g_canRx;

static uint32_t list_ids[]                = { BCAN_ID_DCU_RESET, BCAN_ID_DCU_MIRROR_ORDER };
static const CanFilter f_list             = { CAN_FILTER_LIST, { .list = { list_ids,  2 } } };
static const CanFilter f_any              = { CAN_FILTER_MASK, { .mask = { 0,         0 } } };

int subID_all = -1;
int subID_rx  = -1;
int subID_dcu_mirror_order = -1;
int subID_dcu_reset = -1;

int jobID_pow_mirror_state = -1;
// CAN ========================================================

// PWM ========================================================
Adafruit_PWMServoDriver pwm(0x40);  
// 대략적인 펄스 경계 (필요시 보정)
const int SERVO_MIN = 150; // ≈ 500us
const int SERVO_MAX = 600; // ≈ 2500us

const int PWM_SDA         = 21;
const int PWM_SCL         = 22;
// PWM ========================================================

// FSM ========================================================
enum class State : uint8_t { NotReady = 0, Ready, Busy };

typedef struct {
    int room_pitch_deg;
    int room_yaw_deg;
    int left_pitch_deg;
    int left_yaw_deg;
    int right_pitch_deg;
    int right_yaw_deg;
} ControlStatus;
struct ControlContext {
    ControlStatus target;
    bool active = false;
} g_control;

bool g_isReady = false;
State g_state = State::NotReady;
State g_prevState = State::NotReady;
uint32_t g_stateEnterMs = 0;

static volatile ControlStatus g_defaultStatus = {90, 90, 90, 90, 90, 90};
static volatile ControlStatus g_currentStatus = {90, 90, 90, 90, 90, 90};

void fsm_enterState(State s);

void fsm_onEnterNotReady();
void fsm_onEnterReady();
void fsm_onEnterBusy();

void fsm_stateNotReadyLoop();
void fsm_stateReadyLoop();
void fsm_stateBusyLoop(); 
// FSM ========================================================

// RELAY ======================================================
enum class MirrorDir : uint8_t { OFF=0, UP, DOWN, LEFT, RIGHT };
typedef struct {
  int pin6;
  int pin7;
  int pin8;
} MirrorPins;
const int RELAY_LEFT_FOLD_PLUS                = 13;
const int RELAY_LEFT_FOLD_MINUS               = 14;
const int RELAY_LEFT_PIN_6                    = 16;
const int RELAY_LEFT_PIN_7                    = 17;
const int RELAY_LEFT_PIN_8                    = 18;

const int RELAY_RIGHT_FOLD_PLUS               = 23;
const int RELAY_RIGHT_FOLD_MINUS              = 25;
const int RELAY_RIGHT_PIN_6                   = 26;
const int RELAY_RIGHT_PIN_7                   = 27;
const int RELAY_RIGHT_PIN_8                   = 32;

static const float STROKE_MS = 6500.0f;
static const float MS_PER_DEG = STROKE_MS / 180.0f; // ≈ 36.111...
const MirrorPins LEFT_M = { RELAY_LEFT_PIN_6,  RELAY_LEFT_PIN_7,  RELAY_LEFT_PIN_8  };
const MirrorPins RIGHT_M= { RELAY_RIGHT_PIN_6, RELAY_RIGHT_PIN_7, RELAY_RIGHT_PIN_8 };

// RELAY ======================================================

// CAN Function ========================================================
static void can_print_frame(const char* tag, const CanFrame* f){
    Serial.printf("[%s] id=0x%X dlc=%u flags=0x%X data:",
           tag, (unsigned)f->id, (unsigned)f->dlc, (unsigned)f->flags);
    for (uint8_t i=0;i<f->dlc && i<8;i++) Serial.printf(" %02X", f->data[i]);
    Serial.printf("\n");
}
static void can_on_rx_all(const CanFrame* f, void* user){
    (void)user;
    can_print_frame("RX-ALL", f);
}
static void can_on_rx(const CanFrame* f, void* user) {
    (void)user;
    xQueueSend(g_canRx, f, 0);
}
static void can_build_pow_mirror_state(CanFrame *frame, void* user) {
    (void)user;
    CanMessage msg = {0};
    msg.pow_mirror_state.sig_mirror_left_pitch  = (uint8_t)constrain(g_currentStatus.left_pitch_deg,  0, 180);
    msg.pow_mirror_state.sig_mirror_left_yaw    = (uint8_t)constrain(g_currentStatus.left_yaw_deg,    0, 180);
    msg.pow_mirror_state.sig_mirror_right_pitch = (uint8_t)constrain(g_currentStatus.right_pitch_deg, 0, 180);
    msg.pow_mirror_state.sig_mirror_right_yaw   = (uint8_t)constrain(g_currentStatus.right_yaw_deg,   0, 180);
    msg.pow_mirror_state.sig_mirror_room_pitch  = (uint8_t)constrain(g_currentStatus.room_pitch_deg,  0, 180);
    msg.pow_mirror_state.sig_mirror_room_yaw    = (uint8_t)constrain(g_currentStatus.room_yaw_deg,    0, 180);
    msg.pow_mirror_state.sig_mirror_status      = (uint8_t)g_state;
    *frame = can_encode_bcan(BCAN_ID_POW_MIRROR_STATE, &msg, BCAN_DLC_POW_MIRROR_STATE);
}
static void can_setup(void){
    g_canRx = xQueueCreate(64, sizeof(CanFrame));

    Serial.printf("[POW-Mirror] setup\n");

    if (can_init(CAN_DEVICE_ESP32) != CAN_OK) {
        Serial.printf("can_init failed\n");
        return;
    }

    CanConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.channel     = 0;
    cfg.bitrate     = 500000;      // Node B와 동일
    cfg.samplePoint = 0.750f;
    cfg.sjw         = 1;
    cfg.mode        = CAN_MODE_NORMAL;

    if (can_open(CH, cfg) != CAN_OK) {
        Serial.printf("can_open failed\n");
        return;
    }
    Serial.printf("can_open OK (NORMAL)\n");

    // 모든 프레임 구독
    if (can_subscribe(CH, &subID_all, f_any, can_on_rx_all, NULL) != CAN_OK) 
        Serial.printf("rx all subscribe failed\n");
    else 
        Serial.printf("rx all subscribe completed\n");

    if (can_subscribe(CH, &subID_rx, f_list, can_on_rx, NULL) != CAN_OK) 
        Serial.printf("subscribe failed\n");
    else 
        Serial.printf("subscribe completed\n");

    if (can_register_job_dynamic(CH, &jobID_pow_mirror_state, can_build_pow_mirror_state, NULL, 20) != CAN_OK)
        Serial.printf("pow_mirror_state job register failed\n");
    else 
        Serial.printf("pow_mirror_state job registered: id=%d (every 20ms)\n", jobID_pow_mirror_state);

    Serial.printf("[POW-Mirror] ready\n");
}
// CAN Function ========================================================

// PWM Function ========================================================
static int  pwm_pulse_from_angle(int deg) {
    deg = constrain(deg, 0, 180);
    return map(deg, 0, 180, SERVO_MIN, SERVO_MAX);
}
static void pwm_setAngle(uint8_t ch, int deg) {
    deg = constrain(deg, 0, 180);
    static int last_deg[2] = { -1, -1 };
    if (ch >= 2 || deg == last_deg[ch]) return;
    pwm.setPWM(ch, 0, pwm_pulse_from_angle(deg));
    last_deg[ch] = deg;
    // if (ch == 0) g_room_yaw_deg = deg;
    // if (ch == 1) g_room_pitch_deg = deg;
}
static void pwm_setup(void) {
    Wire.begin(PWM_SDA, PWM_SCL);
    pwm.begin();
    pwm.setPWMFreq(50);  // 서보는 50Hz
}
// PWM Function ========================================================

// REALY Function ======================================================
static inline void relay_write_pins(int p6, int p7, int p8, bool s6, bool s7, bool s8) {
    digitalWrite(p6, s6 ? HIGH : LOW);
    digitalWrite(p7, s7 ? HIGH : LOW);
    digitalWrite(p8, s8 ? HIGH : LOW);
}

static inline void relay_drive(const MirrorPins* m, MirrorDir d) {
    switch(d) {
        case MirrorDir::UP:           relay_write_pins(m->pin6, m->pin7, m->pin8, false, true,  true);  break;
        case MirrorDir::DOWN:         relay_write_pins(m->pin6, m->pin7, m->pin8, true,  false, false); break;
        case MirrorDir::LEFT:         relay_write_pins(m->pin6, m->pin7, m->pin8, false, true,  false); break;
        case MirrorDir::RIGHT:        relay_write_pins(m->pin6, m->pin7, m->pin8, true,  false, true);  break;
        case MirrorDir::OFF: default: relay_write_pins(m->pin6, m->pin7, m->pin8, true,  true,  true);  break;
    }
}

static void relay_setup() {
    pinMode(RELAY_LEFT_PIN_6,  OUTPUT); pinMode(RELAY_LEFT_PIN_7,  OUTPUT); pinMode(RELAY_LEFT_PIN_8,  OUTPUT); 
    pinMode(RELAY_RIGHT_PIN_6, OUTPUT); pinMode(RELAY_RIGHT_PIN_7, OUTPUT); pinMode(RELAY_RIGHT_PIN_8, OUTPUT); 
    relay_drive(&LEFT_M,  MirrorDir::OFF);
    relay_drive(&RIGHT_M, MirrorDir::OFF);
}

// REALY Function ======================================================

// FSM Function ========================================================
void fsm_setup() {
    g_isReady = true;
    fsm_enterState(State::NotReady);
}

void fsm_enterState(State s) {
    g_prevState = g_state;
    g_state = s;
    g_stateEnterMs = millis();

    switch (g_state) {
        case State::NotReady: fsm_onEnterNotReady(); break;
        case State::Ready:    fsm_onEnterReady();    break;
        case State::Busy:     fsm_onEnterBusy();     break;
    }
}

void fsm_loop() {
    if(!g_isReady) return;
    switch (g_state) {
        case State::NotReady: fsm_stateNotReadyLoop(); break;
        case State::Ready:    fsm_stateReadyLoop();    break;
        case State::Busy:     fsm_stateBusyLoop();     break;
    }
}

uint32_t fsm_stateElapsed() {
    return millis() - g_stateEnterMs;
}

// =========================
// 하드웨어 제어 훅 (여러분 환경에 맞게 구현)
// =========================

// NotReady 단계: 준비 루틴 시작 시 1회 호출
void fsm_beginNotReadySequence() {

}

// NotReady 단계: 매 프레임 호출, 준비 조건 충족 여부 반환
bool fsm_updateNotReadyAndCheckReady() {
    CanMessage msg = {0};
    msg.dcu_reset_ack.sig_index  = NODE_IDX;
    msg.dcu_reset_ack.sig_status = 1;
    CanFrame frame = can_encode_bcan(BCAN_ID_DCU_RESET_ACK, &msg, BCAN_DLC_DCU_RESET_ACK);
    if (can_send(CH, frame, 1000) == CAN_OK) {
        Serial.printf("[POW-Mirror] reset end\n");
        return true;
    }
    else  {
        Serial.printf("[POW-Mirror] can ack failed\n");
        return false;
    }
}

void fsm_onEnterNotReady() {
    fsm_stopAllModules();
    fsm_beginNotReadySequence();
}

// Ready 진입 시 1회 호출 (옵션)
void fsm_onEnterReady() {

}

// Busy 진입 시 1회 호출: 목표값 설정/컨트롤 시작
void fsm_onEnterBusy() {
    g_control.active = true;
}

// Busy 단계: 매 프레임 호출, 작업 완료되면 true 반환
bool fsm_updateBusyAndCheckDone() {

    return false;
}

// 안전 정지/리셋 시 호출
void fsm_stopAllModules() {

}

// =========================
// 상태별 핸들러
// =========================

void fsm_setTargetStatus(const CanMessage *message) {
    g_control.target.left_pitch_deg   = message->dcu_mirror_order.sig_mirror_left_pitch;
    g_control.target.left_yaw_deg     = message->dcu_mirror_order.sig_mirror_left_yaw;
    g_control.target.right_pitch_deg  = message->dcu_mirror_order.sig_mirror_right_pitch;
    g_control.target.right_yaw_deg    = message->dcu_mirror_order.sig_mirror_right_yaw;
    g_control.target.room_pitch_deg   = message->dcu_mirror_order.sig_mirror_room_pitch;
    g_control.target.room_yaw_deg     = message->dcu_mirror_order.sig_mirror_room_yaw;
}

// 공통: Reset 즉시 처리
bool fsm_handleMessage() {
    CanFrame canFrame;
    while(xQueueReceive(g_canRx, &canFrame, 0) == pdTRUE) {
        can_msg_bcan_id_t id;
        CanMessage message;
        if(can_decode_bcan(&canFrame, &id, &message) == CAN_OK) {
            switch(id) {
                case BCAN_ID_DCU_RESET:         fsm_enterState(State::NotReady); return true;
                case BCAN_ID_DCU_MIRROR_ORDER:  if(g_state == State::Ready) { fsm_setTargetStatus(&message); fsm_enterState(State::Busy); return true; } break;
            }
        }
    }
    return false;
}

void fsm_stateNotReadyLoop() {
    // 우선 Reset 확인(다른 상태와 정책 일관성 유지)
    if (fsm_handleMessage()) return;

    // 준비 작업 진행
    if (fsm_updateNotReadyAndCheckReady()) {
        fsm_enterState(State::Ready);
        return;
    }
}

void fsm_stateReadyLoop() {
  // 메시지 우선 처리
  if (fsm_handleMessage()) return;
}

void fsm_stateBusyLoop() {
    // Reset 최우선
    if (fsm_handleMessage()) return;

    if (!g_control.active) {
        // 비정상 상태 보호
        fsm_enterState(State::NotReady);
        return;
    }

    if (fsm_updateBusyAndCheckDone()) {
        g_control.active = false;
        fsm_enterState(State::Ready);
        return;
    }
}
// FSM Function ========================================================

// =========================
// ESP32 진입점
// =========================

void setup() {
    Serial.begin(115200);
    delay(100);
    // 통신/버스/모듈 등 공통 초기화
    can_setup();
    pwm_setup();
    relay_setup();
    fsm_setup();
}

void loop() {
    fsm_loop();
    delay(1);
}