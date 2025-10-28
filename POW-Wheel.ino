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
static const char* KEY_POS  = "pos";
static const char* KEY_ANG  = "ang";
static const char* KEY_VER  = "schema";
static const uint16_t NVS_SCHEMA = 1;

// 저장 정책(플래시 마모 방지)
static const uint32_t NV_MIN_SAVE_INTERVAL_MS = 2000; // 최소 2초 간격

static uint32_t g_nvLastSaveMs = 0;

// NVS ========================================================

// CAN ========================================================
static const char* CH = "twai0";
static const int NODE_IDX = 3;
static QueueHandle_t g_canRx;

static uint32_t list_ids[]                = { BCAN_ID_DCU_RESET, BCAN_ID_DCU_WHEEL_ORDER, BCAN_ID_DCU_WHEEL_BUTTON };
static const CanFilter f_list             = { CAN_FILTER_LIST, { .list = { list_ids,  3 } } };
static const CanFilter f_any              = { CAN_FILTER_MASK, { .mask = { 0,         0 } } };

int subID_all = -1;
int subID_rx  = -1;

int jobID_pow_wheel_state = -1;
// CAN ========================================================

// FSM ========================================================
enum class State : uint8_t { NotReady = 0, Ready, Busy };

bool g_isReady = false;
State g_state = State::NotReady;
State g_prevState = State::NotReady;
uint32_t g_stateEnterMs = 0;

int g_btn_angle = 0, g_btn_pos = 0;
static uint32_t g_btn_ts_angle = 0, g_btn_ts_pos = 0;
static const uint32_t BTN_TIMEOUT_MS = 100;

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
    float stroke_toMax_ms; 
    float stroke_toMin_ms;    
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
    .stroke_toMax_ms      = 14000.0f,
    .stroke_toMin_ms      = 14000.0f,
    .onMoveToMin    = { {PIN_POSITION_PLUS, false}, {PIN_POSITION_MINUS, true}  },
    .onMoveToMax    = { {PIN_POSITION_PLUS, true},  {PIN_POSITION_MINUS, false} },
    .onHold         = { {PIN_POSITION_PLUS, false}, {PIN_POSITION_MINUS, false} }
};

AxisConfig CFG_ANGLE = {
    .min_val        = 0,
    .max_val        = 180,
    .def_val        = 90,
    .stroke_toMax_ms      = 6200.0f,
    .stroke_toMin_ms      = 6200.0f,
    .onMoveToMin    = { {PIN_ANGLE_PLUS, false}, {PIN_ANGLE_MINUS, true}  },
    .onMoveToMax    = { {PIN_ANGLE_PLUS, true},  {PIN_ANGLE_MINUS, false} },
    .onHold         = { {PIN_ANGLE_PLUS, false}, {PIN_ANGLE_MINUS, false} }
};

AxisRuntime RT_POSITION = {CFG_POSITION,    0, 0, 0, 0, 0, 0, false, AxisDir::Hold, AxisDir::Hold, AxisPhase::Idle};
AxisRuntime RT_ANGLE    = {CFG_ANGLE,       0, 0, 0, 0, 0, 0, false, AxisDir::Hold, AxisDir::Hold, AxisPhase::Idle};

AxisRuntime* AXES[] = { &RT_POSITION, &RT_ANGLE };
static const int AXES_N = sizeof(AXES)/sizeof(AXES[0]);

static int       g_ax_idx = 0;
static bool      g_ax_running = false;
static uint32_t  g_ax_gap_ms = 0;
static const uint32_t AX_GAP = 80;

int PINS[] = {PIN_POSITION_PLUS, PIN_POSITION_MINUS, PIN_ANGLE_PLUS, PIN_ANGLE_MINUS };
static const int PINS_N = sizeof(PINS)/sizeof(PINS[0]);

static inline float ms_per_unit_for(const AxisRuntime& rt, AxisDir d){
    float span = float(rt.config.max_val - rt.config.min_val);
    if (span <= 0.0f) return AXIS_MIN_SEG_MS;  // 안전 가드
    float stroke = (d == AxisDir::ToMax) ? rt.config.stroke_toMax_ms
                                         : rt.config.stroke_toMin_ms;
    return stroke / span;
}
// RELAY ======================================================

// NVS Function ========================================================
static void nv_begin(){
    prefs.begin(NVS_NS, /*readOnly=*/false);
    // 스키마 버전 없으면 최초 세팅
    uint16_t ver = prefs.getUShort(KEY_VER, 0);
    if (ver != NVS_SCHEMA){
        prefs.clear();
        prefs.putUShort(KEY_VER, NVS_SCHEMA);
    }
}

static void nv_load_currents(){
    RT_POSITION.current = constrain((int)prefs.getShort(KEY_POS), RT_POSITION.config.min_val, RT_POSITION.config.max_val);
    RT_ANGLE.current    = constrain((int)prefs.getShort(KEY_ANG), RT_ANGLE.config.min_val,  RT_ANGLE.config.max_val);
    g_nvLastSaveMs = millis();
}

static void nv_save_currents_if_due(bool force=false){
    uint32_t now = millis();
    bool time_ok  = (now - g_nvLastSaveMs) >= NV_MIN_SAVE_INTERVAL_MS;
    if (force || time_ok){
        prefs.putShort(KEY_POS, (short)RT_POSITION.current);
        prefs.putShort(KEY_ANG, (short)RT_ANGLE.current);
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
    msg.pow_wheel_state.sig_wheel_angle         = (uint8_t)constrain(RT_ANGLE.current,    RT_ANGLE.config.min_val,    RT_ANGLE.config.max_val);
    msg.pow_wheel_state.sig_wheel_position      = (uint8_t)constrain(RT_POSITION.current, RT_POSITION.config.min_val, RT_POSITION.config.max_val);
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
    // if (can_subscribe(CH, &subID_all, f_any, can_on_rx_all, NULL) != CAN_OK) 
    //     Serial.printf("rx all subscribe failed\n");
    // else 
    //     Serial.printf("rx all subscribe completed\n");

    if (can_subscribe(CH, &subID_rx, f_list, can_on_rx, NULL) != CAN_OK) 
        Serial.printf("subscribe failed\n");
    else 
        Serial.printf("subscribe completed\n");

    if (can_register_job_dynamic(CH, &jobID_pow_wheel_state, can_build_pow_wheel_state, NULL, 20) != CAN_OK)
        Serial.printf("pow_wheel_state job register failed\n");
    else 
        Serial.printf("pow_wheel_state job registered: id=%d (every 20ms)\n", jobID_pow_wheel_state);

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

    AxisDir newDir = (delta > 0) ? AxisDir::ToMax : AxisDir::ToMin;

    float k = ms_per_unit_for(rt, newDir);
    uint32_t dur = (uint32_t)(fabsf((float)delta) * k);
    if(dur < AXIS_MIN_SEG_MS) dur = AXIS_MIN_SEG_MS;
    if(dur > AXIS_MAX_SEG_MS) dur = AXIS_MAX_SEG_MS;

    rt.duration_ms = dur;
    rt.start_val = rt.current;
    rt.dir = newDir;
    rt.isActive = true;

    const uint32_t now = millis();

    if(rt.lastApplied == newDir) {
        rt.phase = AxisPhase::Moving;
        rt.phase_ts = now;
        rt.start_ms = now;
    }
    else {
        relay_apply_hold(cfg);
        rt.lastApplied = AxisDir::Hold;
        rt.phase = AxisPhase::PreHold;
        rt.phase_ts = now;
    }
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
    for(int i = 0; i < PINS_N; ++i) pinMode(PINS[i], OUTPUT);
    for(int i = 0; i < AXES_N; ++i) relay_apply_hold(AXES[i]->config);
}

static void relay_seq_start() {
    for(int i = 0; i < AXES_N; ++i) AXES[i]->target = AXES[i]->config.def_val;
    g_ax_idx = 0; g_ax_running = true; g_ax_gap_ms = 0;
    relay_start(*AXES[g_ax_idx]);
}

static bool relay_seq_update() {
    // 갭 상태라면, 갭이 끝났는지 검사해서 다음 축을 시작
    if (!g_ax_running) {
        if (g_ax_idx >= AXES_N) return true; // 모든 축 완료

        if (millis() - g_ax_gap_ms >= AX_GAP) {
            g_ax_running = true;
            relay_start(*AXES[g_ax_idx]); // 다음 축 시작
        }
        return false; // 아직 진행 중
    }

    // 러닝 상태: 현재 축을 진행
    if (relay_update(*AXES[g_ax_idx])) {
        // 현재 축 완료
        g_ax_idx++;
        if (g_ax_idx >= AXES_N) {
            g_ax_running = false;
            return true; // 전체 완료
        }
        // 다음 축 시작 전 갭으로 전환
        g_ax_running = false;
        g_ax_gap_ms = millis();
        return false;
    }

    return false; // 아직 진행 중
}

static inline int jog_step_for(const AxisRuntime& rt, AxisDir d) {
    // 세그먼트가 AXIS_MIN_SEG_MS 이상이 되도록 단위(step) 계산
    float units = AXIS_MIN_SEG_MS / ms_per_unit_for(rt, d);;
    if (units < 1.0f) units = 1.0f;
    int step = (int)ceilf(units);
    // 한 번에 전체 스트로크를 넘지 않도록 안전 제한
    int maxSpan = rt.config.max_val - rt.config.min_val;
    if (step > maxSpan) step = maxSpan;
    return step;
}

static inline int btn_sign(int b) {
    // 0=중립, 1=플러스(+), 2=마이너스(-)
    return (b == 1) ? +1 : (b == 2) ? -1 : 0;
}

static inline int axis_eval_now(const AxisRuntime& rt, uint32_t now) {
    if (!rt.isActive || rt.phase != AxisPhase::Moving || rt.duration_ms == 0) return rt.current;
    float t = (float)(now - rt.start_ms) / (float)rt.duration_ms;
    if (t < 0) t = 0; if (t > 1) t = 1;
    return (int)lroundf((1.0f - t) * rt.start_val + t * (float)rt.target);
}

static inline void relay_abort(AxisRuntime& rt) {
    if (!rt.isActive) return;
    uint32_t now = millis();
    int cur = axis_eval_now(rt, now); // 진행 중이면 현재 위치 보간
    rt.current = cur;
    relay_apply_hold(rt.config);      // 핀 즉시 Hold로
    rt.isActive   = false;
    rt.phase      = AxisPhase::Idle;
    rt.dir        = AxisDir::Hold;
    rt.lastApplied= AxisDir::Hold;    // 다음에 누르면 데드타임 정상 적용
    nv_save_currents_if_due(/*force=*/true); // 선택: 현재값 저장
}

static inline int relay_btn_with_timeout(int btn, uint32_t ts){
  return (millis() - ts > BTN_TIMEOUT_MS) ? 0 : btn;
}

static void relay_button(AxisRuntime& rt, int btn) {
    int s = btn_sign(btn);                // 0, +1, -1
    if (s == 0) {
        relay_abort(rt);  // ★ 즉시 멈춤
        return;
    }

    AxisDir wantDir = (s > 0) ? AxisDir::ToMax : AxisDir::ToMin;

    // ===== 이동 중 "같은 방향"이면 타깃을 연장 (데드타임 스킵) =====
    if (rt.isActive && rt.phase == AxisPhase::Moving) {
        
        if (rt.lastApplied == wantDir) {
            uint32_t now = millis();
            int cur = axis_eval_now(rt, now);
            int step = jog_step_for(rt, wantDir);

            int wantTarget = constrain(rt.target + s * step, rt.config.min_val, rt.config.max_val);
            if (wantTarget != rt.target) {
                // 지금 위치(cur)에서 새 타깃으로 계속 진행 (핀 상태/phase 유지)
                rt.start_val   = cur;
                rt.target      = wantTarget;
                rt.start_ms    = now;
                float k        = ms_per_unit_for(rt, wantDir);
                uint32_t dur   = (uint32_t)lroundf(fabsf((float)(rt.target - cur)) * k);
                if (dur < AXIS_MIN_SEG_MS) dur = AXIS_MIN_SEG_MS;
                if (dur > AXIS_MAX_SEG_MS) dur = AXIS_MAX_SEG_MS;
                rt.duration_ms = dur;
                // phase=Moving / lastApplied = wantDir 그대로 유지
            }
            // 같은 방향 이동 연장 처리 끝
            return;
        }
        // 이동 중인데 방향이 다르면 일단 업데이트 진행
        if (!relay_update(rt)) return;
    } else if (rt.isActive) {
        // Moving이 아니면(PreHold/FinalHold 등) 일단 업데이트
        if (!relay_update(rt)) return;
    }

    // ===== 여기서부터는 비활성(Idle/FinalHold 이후) 또는 갓 완료된 경우: 새 세그먼트 시작 =====
    int step = jog_step_for(rt, wantDir);
    int nextTarget = constrain(rt.current + s * step, rt.config.min_val, rt.config.max_val);
    if (nextTarget == rt.current) return;   // 더 갈 데 없으면 무시

    rt.target = nextTarget;
    relay_start(rt); // relay_start()는 lastApplied==dir이면 PreHold 스킵하고 Moving으로 직행
}

// REALY Function ======================================================
// CMD Function ========================================================

static void cmd_status() {
    const char* stateStr = "";
    switch (g_state) {
        case State::NotReady: stateStr = "NotReady"; break;
        case State::Ready:    stateStr = "Ready";    break;
        case State::Busy:     stateStr = "Busy";     break;
        default:              stateStr = "Unknown";  break;
    }

    Serial.printf("[CMD] State=%s(%d)\n", stateStr, (int)g_state);
    Serial.printf("  - ANGLE:   cur=%d, active=%d\n", RT_ANGLE.current, RT_ANGLE.isActive ? 1 : 0);
    Serial.printf("  - POS: cur=%d, active=%d\n", RT_POSITION.current, RT_POSITION.isActive ? 1 : 0);
    Serial.printf("  - AX seq: idx=%d running=%d\n", g_ax_idx, g_ax_running?1:0);
}

static void cmd_reset_status() {
    // 1) 동작 중이면 안전 중단 (하드웨어는 Hold 유지, 위치 명령 안보냄)
    relay_abort(RT_ANGLE);
    relay_abort(RT_POSITION);
    // 2) 소프트웨어상의 current만 0으로 보정 (하드웨어는 그대로)
    RT_ANGLE.current   = 0;
    RT_POSITION.current = 0;

    // (선택) 원치 않는 후속 이동을 막고 싶다면 target도 0으로 맞추고 싶을 수 있음:
    RT_ANGLE.target = RT_POSITION.target = 0;

    // 3) NVS에 즉시 반영
    nv_save_currents_if_due(/*force=*/true);

    Serial.println("[CMD] Reset Status: all currents set to 0 (software only).");
}

// CMD Function ========================================================

// FSM Function ========================================================
static void serial_poll_commands() {
    // 줄 단위로 읽기(개행까지). 블로킹을 피하려면 available 체크 후 한 줄만 처리.
    if (!Serial.available()) return;

    String line = Serial.readStringUntil('\n');
    line.trim();                        // 앞뒤 공백 제거
    line.toLowerCase();                 // 대소문자 무시

    if (line == "reset status") {
        cmd_reset_status();
    }
    else if (line == "status") {
        cmd_status();
    }
    // 추후 다른 명령도 여기에 추가 가능
}

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

void fsm_setButtonStatus(const CanMessage *message) {
    g_btn_angle = constrain(message->dcu_wheel_button.sig_wheel_angle_button, 0, 2);          g_btn_ts_angle = millis();
    g_btn_pos   = constrain(message->dcu_wheel_button.sig_wheel_position_button, 0, 2);       g_btn_ts_pos   = millis();
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
                case BCAN_ID_DCU_WHEEL_BUTTON: if(g_state == State::Ready) { fsm_setButtonStatus(&message); } break;
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
    // 릴레이 안전 정지
    for(int i = 0; i < AXES_N; ++i) {
        relay_apply_hold(AXES[i]->config);
        AXES[i]->isActive = false;
        AXES[i]->phase = AxisPhase::Idle;
    }

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

            if(relay_done) {
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
                    Serial.printf("[POW-Wheel] homing done, reset ack OK\n");
                    fsm_enterState(State::Ready);
                    return;
                } else {
                    Serial.printf("[POW-Wheel] reset ack failed (try %u)\n", g_ackTries);
                    // (선택) 최대 횟수 초과 시 재초기화/알람 등
                    if (g_ackTries >= ACK_MAX_TRIES) {
                        // 예: CAN 재오픈 시도, 또는 잠시 대기 등
                        // can_open 재시도 로직을 넣을 수 있음
                        g_ackTries = 0; 
                        fsm_enterState(State::Ready);
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

    relay_button(RT_POSITION,   relay_btn_with_timeout(g_btn_pos,   g_btn_ts_pos));
    relay_button(RT_ANGLE,      relay_btn_with_timeout(g_btn_angle, g_btn_ts_angle));

    (void)relay_update(RT_POSITION);
    (void)relay_update(RT_ANGLE);

    serial_poll_commands();
}

void fsm_onEnterBusy() {
    g_ax_idx = 0; g_ax_running = true; g_ax_gap_ms = 0;
    relay_start(*AXES[g_ax_idx]); 
}

void fsm_stateBusyLoop() {
    if (fsm_handleMessage()) return;
    bool relay_done = relay_seq_update();
    if(relay_done) {
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