#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "can_api.h"
#include "canmessage.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <Preferences.h>

// NVS ========================================================
Preferences prefs;

// NVS 키/네임스페이스
static const char* NVS_NS   = "mirror";
static const char* KEY_LY = "l_yaw";
static const char* KEY_LP = "l_ptc";
static const char* KEY_RY = "r_yaw";
static const char* KEY_RP = "r_ptc";
static const char* KEY_RMY= "rm_yaw";
static const char* KEY_RMP= "rm_ptc";
static const char* KEY_VER  = "schema";
static const uint16_t NVS_SCHEMA = 1;

// 저장 정책(플래시 마모 방지)
static const uint32_t NV_MIN_SAVE_INTERVAL_MS = 2000; // 최소 2초 간격

static uint32_t g_nvLastSaveMs = 0;

// NVS ========================================================

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

struct ServoRt {
  uint8_t  ch;              // PCA9685 채널 (0=room_yaw, 1=room_pitch 가정)
  int      current = 90;    // 현재 각도(추적 값)
  int      target  = 90;    // 목표 각도
  int      start_val = 90;  // 이동 시작 시점 각도(보간 기준)
  uint32_t start_ms = 0;    // 이동 시작 시각
  uint32_t duration_ms = 0; // 이번 이동에 걸리는 총 시간
  bool     active = false;  // 진행 중 여부
  float    ms_per_deg = 5000.0f / 180.0f; // ★ 풀스트로크 5초 기준 속도(≈27.78ms/deg)
};
static ServoRt SRV_YAW   { .ch = 0 };
static ServoRt SRV_PITCH { .ch = 1 };
// PWM ========================================================

// FSM ========================================================
enum class State : uint8_t { NotReady = 0, Ready, Busy };

bool g_isReady = false;
State g_state = State::NotReady;
State g_prevState = State::NotReady;
uint32_t g_stateEnterMs = 0;

void fsm_enterState(State s);

void fsm_onEnterNotReady();
void fsm_onEnterReady();
void fsm_onEnterBusy();

void fsm_stateNotReadyLoop();
void fsm_stateReadyLoop();
void fsm_stateBusyLoop(); 
// FSM ========================================================

// RELAY ======================================================

// const int PIN_LEFT_FOLD_PLUS                = 13;
// const int PIN_LEFT_FOLD_MINUS               = 14;
const int PIN_LEFT_PIN_6                    = 16;
const int PIN_LEFT_PIN_7                    = 17;
const int PIN_LEFT_PIN_8                    = 18;

// const int PIN_RIGHT_FOLD_PLUS               = 23;
// const int PIN_RIGHT_FOLD_MINUS              = 25;
const int PIN_RIGHT_PIN_6                   = 26;
const int PIN_RIGHT_PIN_7                   = 27;
const int PIN_RIGHT_PIN_8                   = 32;

static const uint32_t AXIS_MIN_SEG_MS = 120;
static const uint32_t AXIS_MAX_SEG_MS = 7000;
static const uint32_t DIR_DEADTIME_MS = 60;
struct PinState {
    int pinIndex;
    bool pinState;
};
enum class AxisDir : uint8_t { ToMin = 0, ToMax = 1, Hold = 2 };
enum class AxisPhase : uint8_t { Idle, PreHold, Moving, FinalHold };
static const uint8_t MAX_PINS_PER_PATTERN = 3;
struct AxisConfig {
    int min_val, max_val, def_val;
    float stroke_ms, ms_per_unit;
    PinState onMoveToMin[MAX_PINS_PER_PATTERN]; 
    PinState onMoveToMax[MAX_PINS_PER_PATTERN]; 
    PinState onHold[MAX_PINS_PER_PATTERN];
};
struct AxisRuntime {
    AxisConfig config;
    int current = 0, target = 0;
    int start_val = 0;
    uint32_t start_ms = 0, duration_ms = 0;
    uint32_t phase_ts = 0;
    bool isActive = false;
    AxisDir dir = AxisDir::Hold;
    AxisDir lastApplied = AxisDir::Hold;
    AxisPhase phase = AxisPhase::Idle;
};
AxisConfig CFG_LEFT_YAW = {
    .min_val        = 0,
    .max_val        = 180,
    .def_val        = 90,
    .stroke_ms      = 6500.0f,
    .ms_per_unit    = 6500.0f / 180,
    .onMoveToMin    = { {PIN_LEFT_PIN_6, false}, {PIN_LEFT_PIN_7, true}, {PIN_LEFT_PIN_8, false} },
    .onMoveToMax    = { {PIN_LEFT_PIN_6, true}, {PIN_LEFT_PIN_7, false}, {PIN_LEFT_PIN_8, true} },
    .onHold         = { {PIN_LEFT_PIN_6, false}, {PIN_LEFT_PIN_7, false}, {PIN_LEFT_PIN_8, false} }
};
AxisConfig CFG_LEFT_PITCH = {
    .min_val        = 0,
    .max_val        = 180,
    .def_val        = 90,
    .stroke_ms      = 6500.0f,
    .ms_per_unit    = 6500.0f / 180,
    .onMoveToMin    = { {PIN_LEFT_PIN_6, true}, {PIN_LEFT_PIN_7, false}, {PIN_LEFT_PIN_8, false} },
    .onMoveToMax    = { {PIN_LEFT_PIN_6, false}, {PIN_LEFT_PIN_7, true}, {PIN_LEFT_PIN_8, true} },
    .onHold         = { {PIN_LEFT_PIN_6, false}, {PIN_LEFT_PIN_7, false}, {PIN_LEFT_PIN_8, false} }
};

AxisConfig CFG_RIGHT_YAW = {
    .min_val        = 0,
    .max_val        = 180,
    .def_val        = 90,
    .stroke_ms      = 6500.0f,
    .ms_per_unit    = 6500.0f / 180,
    .onMoveToMin    = { {PIN_RIGHT_PIN_6, false}, {PIN_RIGHT_PIN_7, true}, {PIN_RIGHT_PIN_8, false} },
    .onMoveToMax    = { {PIN_RIGHT_PIN_6, true}, {PIN_RIGHT_PIN_7, false}, {PIN_RIGHT_PIN_8, true} },
    .onHold         = { {PIN_RIGHT_PIN_6, false}, {PIN_RIGHT_PIN_7, false}, {PIN_RIGHT_PIN_8, false} }
};
AxisConfig CFG_RIGHT_PITCH = {
    .min_val        = 0,
    .max_val        = 180,
    .def_val        = 90,
    .stroke_ms      = 6500.0f,
    .ms_per_unit    = 6500.0f / 180,
    .onMoveToMin    = { {PIN_RIGHT_PIN_6, true}, {PIN_RIGHT_PIN_7, false}, {PIN_RIGHT_PIN_8, false} },
    .onMoveToMax    = { {PIN_RIGHT_PIN_6, false}, {PIN_RIGHT_PIN_7, true}, {PIN_RIGHT_PIN_8, true} },
    .onHold         = { {PIN_RIGHT_PIN_6, false}, {PIN_RIGHT_PIN_7, false}, {PIN_RIGHT_PIN_8, false} }
};

AxisRuntime RT_LEFT_YAW         = {CFG_LEFT_YAW,     90, 90, 0, 0, 0, 0, false, AxisDir::Hold, AxisDir::Hold, AxisPhase::Idle};
AxisRuntime RT_LEFT_PITCH       = {CFG_LEFT_PITCH,   90, 90, 0, 0, 0, 0, false, AxisDir::Hold, AxisDir::Hold, AxisPhase::Idle};
AxisRuntime RT_RIGHT_YAW        = {CFG_RIGHT_YAW,    90, 90, 0, 0, 0, 0, false, AxisDir::Hold, AxisDir::Hold, AxisPhase::Idle};
AxisRuntime RT_RIGHT_PITCH      = {CFG_RIGHT_PITCH,  90, 90, 0, 0, 0, 0, false, AxisDir::Hold, AxisDir::Hold, AxisPhase::Idle};

AxisRuntime* AXES[] = { &RT_LEFT_YAW, &RT_LEFT_PITCH, &RT_RIGHT_YAW, &RT_RIGHT_PITCH };
static const int AXES_N = sizeof(AXES)/sizeof(AXES[0]);

static int       g_ax_idx = 0;
static bool      g_ax_running = false;
static uint32_t  g_ax_gap_ms = 0;
static const uint32_t AX_GAP = 80;

// RELAY ======================================================

// NVS Function ========================================================
static void nv_begin(){
    prefs.begin(NVS_NS, /*readOnly=*/false);
    // 스키마 버전 없으면 최초 세팅
    uint16_t ver = prefs.getUShort(KEY_VER, 0);
    if (ver != NVS_SCHEMA){
        prefs.putUShort(KEY_VER, NVS_SCHEMA);
    }
}

static void nv_load_currents(){
    RT_LEFT_YAW.current    = prefs.getShort(KEY_LY,  (short)RT_LEFT_YAW.config.def_val);
    RT_LEFT_PITCH.current  = prefs.getShort(KEY_LP,  (short)RT_LEFT_PITCH.config.def_val);
    RT_RIGHT_YAW.current   = prefs.getShort(KEY_RY,  (short)RT_RIGHT_YAW.config.def_val);
    RT_RIGHT_PITCH.current = prefs.getShort(KEY_RP,  (short)RT_RIGHT_PITCH.config.def_val);
    SRV_YAW.current         = prefs.getShort(KEY_RMY, (short)90);
    SRV_PITCH.current       = prefs.getShort(KEY_RMP, (short)90);
    g_nvLastSaveMs = millis();
}

static void nv_save_currents_if_due(bool force=false){
    uint32_t now = millis();
    bool time_ok  = (now - g_nvLastSaveMs) >= NV_MIN_SAVE_INTERVAL_MS;
    if (force || time_ok){
        prefs.putShort(KEY_LY,  (short)RT_LEFT_YAW.current);
        prefs.putShort(KEY_LP,  (short)RT_LEFT_PITCH.current);
        prefs.putShort(KEY_RY,  (short)RT_RIGHT_YAW.current);
        prefs.putShort(KEY_RP,  (short)RT_RIGHT_PITCH.current);
        prefs.putShort(KEY_RMY, (short)SRV_YAW.current);
        prefs.putShort(KEY_RMP, (short)SRV_PITCH.current);
        g_nvLastSaveMs = now;
    }
}

static void nv_setup() {
    nv_begin();
    nv_load_currents();
}
// NVS Function ========================================================


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
    msg.pow_mirror_state.sig_mirror_left_pitch  = (uint8_t)constrain(RT_LEFT_PITCH.current,  RT_LEFT_PITCH.config.min_val, RT_LEFT_PITCH.config.max_val);
    msg.pow_mirror_state.sig_mirror_left_yaw    = (uint8_t)constrain(RT_LEFT_YAW.current,   RT_LEFT_YAW.config.min_val, RT_LEFT_YAW.config.max_val);
    msg.pow_mirror_state.sig_mirror_right_pitch = (uint8_t)constrain(RT_RIGHT_PITCH.current,  RT_RIGHT_PITCH.config.min_val, RT_RIGHT_PITCH.config.max_val);
    msg.pow_mirror_state.sig_mirror_right_yaw   = (uint8_t)constrain(RT_RIGHT_YAW.current,  RT_RIGHT_YAW.config.min_val, RT_RIGHT_YAW.config.max_val);
    msg.pow_mirror_state.sig_mirror_room_pitch  = (uint8_t)constrain(SRV_PITCH.current,  0, 180);
    msg.pow_mirror_state.sig_mirror_room_yaw    = (uint8_t)constrain(SRV_YAW.current,    0, 180);
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
    static int last_deg[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
    if (ch >= 16 || deg == last_deg[ch]) return;
    pwm.setPWM(ch, 0, pwm_pulse_from_angle(deg));
    last_deg[ch] = deg;
}
static inline uint32_t pwm_duration_for(int from_deg, int to_deg, float ms_per_deg){
  int d = abs(to_deg - from_deg);
  uint32_t dur = (uint32_t)lroundf(d * ms_per_deg);
  if (dur < 20)  dur = 20;        // 너무 짧은 펄스 방지
  if (dur > 8000) dur = 8000;     // 안전 상한(선택)
  return dur;
}
static inline int pwm_eval_now(const ServoRt& s, uint32_t now){
  if (!s.active || s.duration_ms == 0) return s.current;
  float t = (float)(now - s.start_ms) / (float)s.duration_ms;
  if (t < 0) t = 0; if (t > 1) t = 1;
  return (int)lroundf((1.0f - t) * s.start_val + t * (float)s.target);
}
static void pwm_setup(void) {
    Wire.begin(PWM_SDA, PWM_SCL);
    pwm.begin();
    pwm.setPWMFreq(50);  // 서보는 50Hz
}
static void pwm_start(ServoRt& s, int newTarget){
    newTarget = constrain(newTarget, 0, 180);
    uint32_t now = millis();

    // 진행 중이면 현재 진행 위치를 기준으로 리타겟
    int from = s.active ? pwm_eval_now(s, now) : s.current;

    if (from == newTarget){
        s.current = s.target = newTarget;
        s.active = false;
        pwm_setAngle(s.ch, newTarget);   // 스냅
        return;
    }

    s.start_val   = from;
    s.target      = newTarget;
    s.start_ms    = now;
    s.duration_ms = pwm_duration_for(from, newTarget, s.ms_per_deg);
    s.active      = true;

    // 바로 1틱 적용해도 됨(선택)
    pwm_setAngle(s.ch, from);
}
static bool pwm_update(ServoRt& s){
    if (!s.active) return true;
    uint32_t now = millis();

    uint32_t elapsed = now - s.start_ms;
    float t = (s.duration_ms == 0) ? 1.0f : (float)elapsed / (float)s.duration_ms;
    if (t > 1.0f) t = 1.0f;

    int cur = (int)lroundf((1.0f - t) * s.start_val + t * (float)s.target);
    cur = constrain(cur, 0, 180);
    s.current = cur;
    pwm_setAngle(s.ch, cur);

    if (elapsed >= s.duration_ms){
        s.active = false;
        s.current = s.target;
        pwm_setAngle(s.ch, s.target);  // 최종 스냅
        return true;
    }
    return false;
}
static inline bool pwm_all_done() {
    return !SRV_YAW.active && !SRV_PITCH.active;
}
// PWM Function ========================================================

// REALY Function ======================================================
static void relay_apply_pins(const PinState pins[], uint8_t n = MAX_PINS_PER_PATTERN) {
    for(uint8_t i = 0; i < n; ++i) {
        if(pins[i].pinIndex <= 0) continue;
        digitalWrite(pins[i].pinIndex, pins[i].pinState);
    }
}

static void relay_apply_hold(const AxisConfig& cfg) { relay_apply_pins(cfg.onHold); }

static void relay_apply_dir(const AxisConfig& cfg, AxisDir d) {
    switch(d) {
        case AxisDir::ToMax: relay_apply_pins(cfg.onMoveToMax); break;
        case AxisDir::ToMin: relay_apply_pins(cfg.onMoveToMin); break;
        case AxisDir::Hold: default: relay_apply_hold(cfg);     break;
    }
}

static void relay_start(AxisRuntime& rt) {
    AxisConfig& cfg = rt.config;
    rt.target = constrain(rt.target, cfg.min_val, cfg.max_val);
    int delta = rt.target - rt.current;

    if(delta == 0) {
        rt.isActive = false;
        rt.dir = AxisDir::Hold;
        rt.phase = AxisPhase::Idle;
        relay_apply_hold(cfg);
        return;
    }

    float k = cfg.ms_per_unit;
    uint32_t dur = (uint32_t)(fabsf((float)delta) * k);
    if(dur < AXIS_MIN_SEG_MS) dur = AXIS_MIN_SEG_MS;
    if(dur > AXIS_MAX_SEG_MS) dur = AXIS_MAX_SEG_MS;

    rt.duration_ms = dur;
    rt.start_ms = millis();
    rt.start_val = rt.current;
    rt.dir = (delta > 0) ? AxisDir::ToMax : AxisDir::ToMin;
    rt.isActive = true;

    relay_apply_hold(cfg);
    rt.lastApplied = AxisDir::Hold;
    rt.phase = AxisPhase::PreHold;
    rt.phase_ts = millis();
}

static bool relay_update(AxisRuntime& rt) {
    if (!rt.isActive) return true;

    const uint32_t now = millis();

    switch(rt.phase) {
        case AxisPhase::PreHold: {
            if(now - rt.phase_ts >= DIR_DEADTIME_MS) {
                relay_apply_dir(rt.config, rt.dir);
                rt.lastApplied = rt.dir;
                rt.phase = AxisPhase::Moving;
                rt.phase_ts = now;
                rt.start_ms = now;
            }
            break;
        }
        case AxisPhase::Moving: {
            const uint32_t elapsed = now - rt.start_ms;
            float t = (rt.duration_ms == 0) ? 1.0f : (float)elapsed / (float)rt.duration_ms;
            if(t > 1.0f) t = 1.0f;
            const int newCur = (int)lroundf((1.0f - t) * rt.start_val + t * (float)rt.target);
            rt.current = constrain(newCur, rt.config.min_val, rt.config.max_val);

            if(elapsed >= rt.duration_ms) {
                rt.current = rt.target;
                relay_apply_hold(rt.config);
                rt.lastApplied = AxisDir::Hold;
                rt.phase = AxisPhase::FinalHold;
                rt.phase_ts = now;
            }
            break;
        }
        case AxisPhase::FinalHold: 
            rt.isActive = false;
            rt.dir      = AxisDir::Hold;
            rt.phase    = AxisPhase::Idle;
            nv_save_currents_if_due(/*force=*/true);
            return true;
        case AxisPhase::Idle:
            return true;
    }
    return false;
}

static void relay_setup() {
    pinMode(PIN_LEFT_PIN_6,  OUTPUT);   pinMode(PIN_LEFT_PIN_7,  OUTPUT);  pinMode(PIN_LEFT_PIN_8,  OUTPUT);
    pinMode(PIN_RIGHT_PIN_6, OUTPUT);   pinMode(PIN_RIGHT_PIN_7, OUTPUT);  pinMode(PIN_RIGHT_PIN_8, OUTPUT);
    relay_apply_hold(CFG_LEFT_PITCH);   relay_apply_hold(CFG_LEFT_YAW);
    relay_apply_hold(CFG_RIGHT_PITCH);  relay_apply_hold(CFG_RIGHT_YAW);
}

static void relay_seq_start() {
    for(int i = 0; i < AXES_N; ++i) AXES[i]->target = AXES[i]->config.def_val;
    g_ax_idx = 0; g_ax_running = true; g_ax_gap_ms = 0;
    relay_start(*AXES[g_ax_idx]);
}

static bool relay_seq_update() {
    if (!g_ax_running) return true;

    if(relay_update(*AXES[g_ax_idx])) {
        g_ax_idx++;
        if(g_ax_idx >= AXES_N) {g_ax_running = false; return true;}
        g_ax_gap_ms = millis();
    }

    if(!g_ax_running && g_ax_idx < AXES_N) {
        if(millis() - g_ax_gap_ms >= AX_GAP) {
            g_ax_running = true;
            relay_start(*AXES[g_ax_idx]);
        }
    }

    return false;
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

void fsm_setTargetStatus(const CanMessage *message) {
    RT_LEFT_YAW.target      = constrain(message->dcu_mirror_order.sig_mirror_left_yaw, RT_LEFT_YAW.config.min_val, RT_LEFT_YAW.config.max_val);
    RT_LEFT_PITCH.target    = constrain(message->dcu_mirror_order.sig_mirror_left_pitch, RT_LEFT_PITCH.config.min_val, RT_LEFT_PITCH.config.max_val);
    RT_RIGHT_YAW.target     = constrain(message->dcu_mirror_order.sig_mirror_right_yaw, RT_RIGHT_YAW.config.min_val, RT_RIGHT_YAW.config.max_val);
    RT_RIGHT_PITCH.target   = constrain(message->dcu_mirror_order.sig_mirror_right_pitch, RT_RIGHT_PITCH.config.min_val, RT_RIGHT_PITCH.config.max_val);
    SRV_YAW.target          = constrain(message->dcu_mirror_order.sig_mirror_room_yaw, 0, 180);
    SRV_PITCH.target        = constrain(message->dcu_mirror_order.sig_mirror_room_pitch, 0, 180);
}

bool fsm_handleMessage() {
    CanFrame canFrame;
    while(xQueueReceive(g_canRx, &canFrame, 0) == pdTRUE) {
        can_msg_bcan_id_t id;
        CanMessage message;
        if(can_decode_bcan(&canFrame, &id, &message) == CAN_OK) {
            switch(id) {
                case BCAN_ID_DCU_RESET:        fsm_enterState(State::NotReady); return true;
                case BCAN_ID_DCU_MIRROR_ORDER:  if(g_state == State::Ready) { fsm_setTargetStatus(&message); fsm_enterState(State::Busy); return true; } break;
            }
        }
    }
    return false;
}

// FSM Function ========================================================

// FSM State  ==========================================================

enum class NRPhase : uint8_t { Homing = 0, AckPending };

static NRPhase   g_nrPhase         = NRPhase::Homing;

static uint32_t  g_ackLastTryMs    = 0;
static const uint32_t ACK_RETRY_MS  = 300;   // 재시도 간격
static uint8_t   g_ackTries        = 0;
static const uint8_t ACK_MAX_TRIES = 10;     // 필요시 제한(선택)

void fsm_onEnterNotReady() {
    pwm_start(SRV_YAW,      90);
    pwm_start(SRV_PITCH,    90);
    // 릴레이 안전 정지
    relay_apply_hold(RT_LEFT_PITCH.config); relay_apply_hold(RT_LEFT_YAW.config);
    relay_apply_hold(RT_RIGHT_YAW.config);  relay_apply_hold(RT_RIGHT_PITCH.config);
    // 축 상태 초기화
    RT_LEFT_YAW.isActive = RT_LEFT_PITCH.isActive = false;
    RT_RIGHT_YAW.isActive = RT_RIGHT_PITCH.isActive = false;
    RT_LEFT_YAW.phase = RT_LEFT_PITCH.phase = AxisPhase::Idle;
    RT_RIGHT_YAW.phase = RT_RIGHT_PITCH.phase = AxisPhase::Idle;

    relay_seq_start();

    // 시퀀스 초기화: Position부터
    g_nrPhase      = NRPhase::Homing;
    g_ackLastTryMs = 0; g_ackTries = 0;
}

void fsm_stateNotReadyLoop() {
    // 리셋 등 우선 처리
    if (fsm_handleMessage()) return;

    switch (g_nrPhase) {
        case NRPhase::Homing: {
            bool relay_done = relay_seq_update();
            bool pwm_done   = pwm_all_done();

            if(relay_done && pwm_done) {
                g_nrPhase = NRPhase::AckPending;
                g_ackLastTryMs = 0;     g_ackTries = 0;
            }
            break;
        }
        case NRPhase::AckPending: {
            const uint32_t now = millis();
            if (g_ackLastTryMs == 0 || (now - g_ackLastTryMs) >= ACK_RETRY_MS) {
                g_ackLastTryMs = now;
                g_ackTries++;

                CanMessage msg = {0};
                msg.dcu_reset_ack.sig_index  = NODE_IDX;
                msg.dcu_reset_ack.sig_status = 1;
                CanFrame frame = can_encode_bcan(BCAN_ID_DCU_RESET_ACK, &msg, BCAN_DLC_DCU_RESET_ACK);

                if (can_send(CH, frame, 1000) == CAN_OK) {
                    Serial.printf("[POW-Mirror] homing done, reset ack OK\n");
                    fsm_enterState(State::Ready);
                    return;
                } else {
                    Serial.printf("[POW-Mirror] reset ack failed (try %u)\n", g_ackTries);
                    // (선택) 최대 횟수 초과 시 재초기화/알람 등
                    if (g_ackTries >= ACK_MAX_TRIES) {
                        // 예: CAN 재오픈 시도, 또는 잠시 대기 등
                        // can_open 재시도 로직을 넣을 수 있음
                        g_ackTries = 0; // 계속 시도할 거라면 카운터 리셋
                    }
                }
            }
            break;
        }
    }
}

void fsm_onEnterReady() {
         
}

void fsm_stateReadyLoop() {
    if (fsm_handleMessage()) return;


}

void fsm_onEnterBusy() {
    pwm_start(SRV_YAW,      SRV_YAW.target);
    pwm_start(SRV_PITCH,    SRV_PITCH.target);
    g_ax_idx = 0; g_ax_running = true; g_ax_gap_ms = 0;
    relay_start(*AXES[g_ax_idx]); 
}

void fsm_stateBusyLoop() {
    if (fsm_handleMessage()) return;
    bool relay_done = relay_seq_update();
    bool pwm_done   = pwm_all_done();
    if(relay_done && pwm_done) {
        fsm_enterState(State::Ready);
    }
}

// FSM State  ==========================================================

// =========================
// ESP32 진입점
// =========================

void setup() {
    Serial.begin(115200);
    delay(100);
    nv_setup();
    can_setup();
    pwm_setup();
    relay_setup();
    fsm_setup();
}

void loop() {
    fsm_loop();
    pwm_update(SRV_YAW);
    pwm_update(SRV_PITCH);
    nv_save_currents_if_due(/*force=*/false); // ★ 선택: 주기적 저장
    delay(1);
}