#include <Arduino.h>
#include "can_api.h"
#include "canmessage.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <Preferences.h>

// NVS ========================================================
Preferences prefs;

// NVS 키/네임스페이스
static const char* NVS_NS   = "wheel";
static const char* KEY_POS  = "pos_cur";
static const char* KEY_ANG  = "ang_cur";
static const char* KEY_VER  = "schema";
static const uint16_t NVS_SCHEMA = 1;

// 저장 정책(플래시 마모 방지)
static const uint32_t NV_MIN_SAVE_INTERVAL_MS = 2000; // 최소 2초 간격
static const int      NV_MIN_DELTA            = 1;    // 값 변화 1 이상일 때만

static uint32_t g_nvLastSaveMs = 0;
static int g_nvLastPos = -32768;
static int g_nvLastAng = -32768;

// NVS ========================================================

// CAN ========================================================
static const char* CH = "twai0";
static const int NODE_IDX = 3;
static QueueHandle_t g_canRx;

static uint32_t list_ids[]                = { BCAN_ID_DCU_RESET, BCAN_ID_DCU_WHEEL_ORDER };
static const CanFilter f_list             = { CAN_FILTER_LIST, { .list = { list_ids,  2 } } };
static const CanFilter f_any              = { CAN_FILTER_MASK, { .mask = { 0,         0 } } };

int subID_all = -1;
int subID_rx  = -1;
int subID_dcu_mirror_order = -1;
int subID_dcu_reset = -1;

int jobID_pow_mirror_state = -1;
// CAN ========================================================

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
const int PIN_POSITION_PLUS         = 13;
const int PIN_POSITION_MINUS        = 14;
const int PIN_ANGLE_PLUS            = 26;
const int PIN_ANGLE_MINUS           = 27;
static const uint32_t AXIS_MIN_SEG_MS = 120;
static const uint32_t AXIS_MAX_SEG_MS = 7000;
static const uint32_t DIR_DEADTIME_MS = 60;
struct PinState {
    int pinIndex;
    bool pinState;
};

enum class AxisDir : uint8_t { ToMin = 0, ToMax = 1, Hold = 2 };
enum class AxisPhase : uint8_t { Idle, PreHold, Moving, FinalHold };
static const uint8_t MAX_PINS_PER_PATTERN = 2;
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

AxisConfig CFG_POSITION = {
    .min_val        = 0,
    .max_val        = 100,
    .def_val        = 50,
    .stroke_ms      = 6500.0f,
    .ms_per_unit    = 6500.0f / 100,
    .onMoveToMin    = { {PIN_POSITION_PLUS, false}, {PIN_POSITION_MINUS, true}  },
    .onMoveToMax    = { {PIN_POSITION_PLUS, true},  {PIN_POSITION_MINUS, false} },
    .onHold         = { {PIN_POSITION_PLUS, false}, {PIN_POSITION_MINUS, false} }
};

AxisConfig CFG_ANGLE = {
    .min_val        = 0,
    .max_val        = 180,
    .def_val        = 90,
    .stroke_ms      = 6500.0f,
    .ms_per_unit    = 6500.0f / 180,
    .onMoveToMin    = { {PIN_ANGLE_PLUS, false}, {PIN_ANGLE_MINUS, true}  },
    .onMoveToMax    = { {PIN_ANGLE_PLUS, true},  {PIN_ANGLE_MINUS, false} },
    .onHold         = { {PIN_ANGLE_PLUS, false}, {PIN_ANGLE_MINUS, false} }
};

AxisRuntime RT_POSITION = {CFG_POSITION,    50, 50, 0, 0, 0, 0, false, AxisDir::Hold, AxisDir::Hold, AxisPhase::Idle};
AxisRuntime RT_ANGLE    = {CFG_ANGLE,       90, 90, 0, 0, 0, 0, false, AxisDir::Hold, AxisDir::Hold, AxisPhase::Idle};
// RELAY ======================================================

// NVS Function ========================================================
static void nv_begin(){
    prefs.begin(NVS_NS, /*readOnly=*/false);
    // 스키마 버전 없으면 최초 세팅
    uint16_t ver = prefs.getUShort(KEY_VER, 0);
    if (ver != NVS_SCHEMA){
        prefs.putUShort(KEY_VER, NVS_SCHEMA);
        // 초기값 강제 기록
        prefs.putShort(KEY_POS, (short)RT_POSITION.current);
        prefs.putShort(KEY_ANG, (short)RT_ANGLE.current);
    }
}

static void nv_load_currents(){
    // 저장값이 있으면 읽고 클램프
    short pos = prefs.getShort(KEY_POS, (short)RT_POSITION.config.def_val);
    short ang = prefs.getShort(KEY_ANG, (short)RT_ANGLE.config.def_val);
    RT_POSITION.current = constrain((int)pos, RT_POSITION.config.min_val, RT_POSITION.config.max_val);
    RT_ANGLE.current    = constrain((int)ang, RT_ANGLE.config.min_val,  RT_ANGLE.config.max_val);

    // 부팅 직후 상태 표시 초기값으로만 사용
    g_nvLastPos = RT_POSITION.current;
    g_nvLastAng = RT_ANGLE.current;
    g_nvLastSaveMs = millis();
}

static void nv_save_currents_if_due(bool force=false){
    uint32_t now = millis();
    int pos = RT_POSITION.current;
    int ang = RT_ANGLE.current;

    bool delta_ok = (abs(pos - g_nvLastPos) >= NV_MIN_DELTA) || (abs(ang - g_nvLastAng) >= NV_MIN_DELTA);
    bool time_ok  = (now - g_nvLastSaveMs) >= NV_MIN_SAVE_INTERVAL_MS;

    if (force || (delta_ok && time_ok)){
        prefs.putShort(KEY_POS, (short)pos);
        prefs.putShort(KEY_ANG, (short)ang);
        g_nvLastPos = pos;
        g_nvLastAng = ang;
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
static void can_build_pow_wheel_state(CanFrame *frame, void* user) {
    (void)user;
    CanMessage msg = {0};
    msg.pow_wheel_state.sig_wheel_angle         = RT_ANGLE.current;
    msg.pow_wheel_state.sig_wheel_position      = RT_POSITION.current;
    msg.pow_wheel_state.sig_wheel_status        = (uint8_t)g_state;
    *frame = can_encode_bcan(BCAN_ID_POW_WHEEL_STATE, &msg, BCAN_DLC_POW_WHEEL_STATE);
}
static void can_setup(void){
    g_canRx = xQueueCreate(64, sizeof(CanFrame));

    Serial.printf("[POW-Wheel] setup\n");

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

    if (can_register_job_dynamic(CH, &jobID_pow_mirror_state, can_build_pow_wheel_state, NULL, 20) != CAN_OK)
        Serial.printf("pow_wheel_state job register failed\n");
    else 
        Serial.printf("pow_wheel_state job registered: id=%d (every 20ms)\n", jobID_pow_mirror_state);

    Serial.printf("[POW-Wheel] ready\n");
}
// CAN Function ========================================================

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
    pinMode(PIN_POSITION_PLUS,  OUTPUT);
    pinMode(PIN_POSITION_MINUS, OUTPUT);
    pinMode(PIN_ANGLE_PLUS,     OUTPUT);
    pinMode(PIN_ANGLE_MINUS,    OUTPUT);
    relay_apply_hold(CFG_POSITION);
    relay_apply_hold(CFG_ANGLE);
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
    RT_POSITION.target  = constrain(message->dcu_wheel_order.sig_wheel_position, RT_POSITION.config.min_val, RT_POSITION.config.max_val);
    RT_ANGLE.target     = constrain(message->dcu_wheel_order.sig_wheel_angle, RT_ANGLE.config.min_val, RT_ANGLE.config.max_val);
}

bool fsm_handleMessage() {
    CanFrame canFrame;
    while(xQueueReceive(g_canRx, &canFrame, 0) == pdTRUE) {
        can_msg_bcan_id_t id;
        CanMessage message;
        if(can_decode_bcan(&canFrame, &id, &message) == CAN_OK) {
            switch(id) {
                case BCAN_ID_DCU_RESET:        fsm_enterState(State::NotReady); return true;
                case BCAN_ID_DCU_WHEEL_ORDER:  if(g_state == State::Ready) { fsm_setTargetStatus(&message); fsm_enterState(State::Busy); return true; } break;
            }
        }
    }
    return false;
}

// FSM Function ========================================================

// FSM State  ==========================================================

enum class NRPhase : uint8_t { Homing = 0, AckPending };

static NRPhase   g_nrPhase         = NRPhase::Homing;
static bool      g_nrPosDone       = true, g_nrAngDone = true;
static uint32_t  g_nrGapStartMs    = 0;
static const uint32_t NR_NEXT_AXIS_DELAY_MS = 80;

static uint32_t  g_ackLastTryMs    = 0;
static const uint32_t ACK_RETRY_MS  = 300;   // 재시도 간격
static uint8_t   g_ackTries        = 0;
static const uint8_t ACK_MAX_TRIES = 10;     // 필요시 제한(선택)

void fsm_onEnterNotReady() {
    relay_apply_hold(RT_POSITION.config);
    relay_apply_hold(RT_ANGLE.config);
    RT_POSITION.isActive = RT_ANGLE.isActive = false;
    RT_POSITION.phase = RT_ANGLE.phase = AxisPhase::Idle;
    RT_POSITION.dir = RT_ANGLE.dir = AxisDir::Hold;

    // 기본값 설정(중간값)
    RT_POSITION.target = RT_POSITION.config.def_val;
    RT_ANGLE.target    = RT_ANGLE.config.def_val;

    // 시퀀스 초기화: Position부터
    g_nrPhase      = NRPhase::Homing;
    g_nrPosDone    = false;
    g_nrAngDone    = true;  // Position 먼저
    g_nrGapStartMs = 0;

    g_ackLastTryMs = 0;
    g_ackTries     = 0;

    relay_start(RT_POSITION);
}

void fsm_stateNotReadyLoop() {
    // 리셋 등 우선 처리
    if (fsm_handleMessage()) return;

    switch (g_nrPhase) {
        case NRPhase::Homing: {
            // 1) Position 진행
            if (!g_nrPosDone) {
                if (relay_update(RT_POSITION)) {
                    g_nrPosDone = true;
                    g_nrGapStartMs = millis();
                }
                return;
            }
            // 2) Gap 후 Angle 시작
            if (g_nrPosDone && g_nrAngDone) {
                if (millis() - g_nrGapStartMs >= NR_NEXT_AXIS_DELAY_MS) {
                    g_nrAngDone = false;
                    relay_start(RT_ANGLE);
                }
                return;
            }
            // 3) Angle 진행
            if (!g_nrAngDone) {
                if (relay_update(RT_ANGLE)) {
                    g_nrAngDone = true;
                }
                return;
            }
            // 4) 둘 다 끝났으면 ACK 단계로 전환
            if (g_nrPosDone && g_nrAngDone) {
                g_nrPhase      = NRPhase::AckPending;
                g_ackLastTryMs = 0;     // 즉시 1회 시도 가능
                g_ackTries     = 0;
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
                    Serial.printf("[POW-Wheel] homing done, reset ack OK\n");
                    fsm_enterState(State::Ready);
                    return;
                } else {
                    Serial.printf("[POW-Wheel] reset ack failed (try %u)\n", g_ackTries);
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

static const uint32_t NEXT_AXIS_DELAY_MS = 80;
static bool g_posDone = true, g_angDone = true;
static uint32_t g_axisGapStartMs = 0;

void fsm_onEnterBusy() {
    g_posDone = false; g_angDone = true;   
    relay_start(RT_POSITION);    
}

void fsm_stateBusyLoop() {
    if (fsm_handleMessage()) return;

    if (!g_posDone){
        if (relay_update(RT_POSITION)){
            g_posDone = true;
            g_axisGapStartMs = millis();
        }
        return;
    }

    if (g_posDone && g_angDone){       
        if (millis() - g_axisGapStartMs >= NEXT_AXIS_DELAY_MS){
            g_angDone = false;
            relay_start(RT_ANGLE);
        }
        return;
    }

    if (!g_angDone){
        if (relay_update(RT_ANGLE)){
            g_angDone = true;
        }
        return;
    }

    if (g_posDone && g_angDone){
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
    relay_setup();
    fsm_setup();
}

void loop() {
    fsm_loop();
    nv_save_currents_if_due(/*force=*/false); // ★ 선택: 주기적 저장
    delay(1);
}