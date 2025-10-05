#pragma once
#include "driver/gpio.h"

/* =============================================================================
 *  CONFIG — 보드/핀/타이밍 설정
 * =============================================================================
 */

/* ---- CAN (TWAI) ---- */
#define CAN_TX_PIN   GPIO_NUM_5
#define CAN_RX_PIN   GPIO_NUM_4
#define CAN_BITRATE  500000   // 500 kbps

/* ---- 릴레이 채널 (회로도 역할명으로 명명) ----
 * 각 채널은 ULN2803(싱크 드라이버)를 통해 "릴레이 코일 1개"를 제어한다.
 *  - 코일 + : +12V
 *  - 코일 - : ULN2803 OUTn
 *  - ULN2803 COM : +12V (내장 플라이백 다이오드 공통)
 *  - GND : ESP32/ULN/12V 공통 접지
 */
enum Axis : uint8_t { SLIDE=0, FRONT=1, REAR=2, RECLINE=3, AXIS_COUNT=4 };

#define RLY_CH_SLIDE_FWD      0
#define RLY_CH_SLIDE_BACK     1
#define RLY_CH_FRONT_UP       2
#define RLY_CH_FRONT_DOWN     3
#define RLY_CH_REAR_UP        4
#define RLY_CH_REAR_DOWN      5
#define RLY_CH_RECLINE_FWD    6
#define RLY_CH_RECLINE_BACK   7

/* ---- 릴레이 채널별 ESP32 GPIO 매핑 ---- */
#define PIN_RLY_SLIDE_FWD     GPIO_NUM_16
#define PIN_RLY_SLIDE_BACK    GPIO_NUM_17
#define PIN_RLY_FRONT_UP      GPIO_NUM_18
#define PIN_RLY_FRONT_DOWN    GPIO_NUM_19
#define PIN_RLY_REAR_UP       GPIO_NUM_21
#define PIN_RLY_REAR_DOWN     GPIO_NUM_22
#define PIN_RLY_RECLINE_FWD   GPIO_NUM_23
#define PIN_RLY_RECLINE_BACK  GPIO_NUM_25

/* 채널 인덱스 순서(RLY_CH_*)와 동일한 물리 핀 테이블 */
static const gpio_num_t RELAY_PINS[8] = {
  PIN_RLY_SLIDE_FWD,    // 0
  PIN_RLY_SLIDE_BACK,   // 1
  PIN_RLY_FRONT_UP,     // 2
  PIN_RLY_FRONT_DOWN,   // 3
  PIN_RLY_REAR_UP,      // 4
  PIN_RLY_REAR_DOWN,    // 5
  PIN_RLY_RECLINE_FWD,  // 6
  PIN_RLY_RECLINE_BACK  // 7
};

/* ---- 축별 H-브리지 페어링 (A/B 릴레이) ----
 * A = 정방향/상승(Forward/Up), B = 역방향/하강(Back/Down)
 */
#define SLIDE_RLY_A   RLY_CH_SLIDE_FWD
#define SLIDE_RLY_B   RLY_CH_SLIDE_BACK
#define FRONT_RLY_A   RLY_CH_FRONT_UP
#define FRONT_RLY_B   RLY_CH_FRONT_DOWN
#define REAR_RLY_A    RLY_CH_REAR_UP
#define REAR_RLY_B    RLY_CH_REAR_DOWN
#define RECL_RLY_A    RLY_CH_RECLINE_FWD
#define RECL_RLY_B    RLY_CH_RECLINE_BACK

/* ---- 태스크 주기 및 릴레이 데드타임 ---- */
#define CTRL_TICK_MS         10    // 제어 루프 주기
#define REPORT_TICK_MS       50    // 상태 전송 주기(약 20 Hz)
#define HBRIDGE_DEADTIME_MS  80    // 브레이크-메이크 간격(ms)

/* ---- 오픈루프 위치 스케일(선택) ---- */
#define POS_MIN              0
#define POS_MAX              1000
#define POS_DEADBAND         5

/* ---- 릴레이 ON일 때의 오픈루프 "속도" (10ms당 tick 수) ----
 * 단위: [ticks / CTRL_TICK_MS]. 만약 CTRL_TICK_MS=10이면,
 * 값이 1일 때 초당 100 tick 증가하는 셈이다.
 * (릴레이는 PWM을 쓰지 않으므로, 데모/오픈루프용 근사치 속도)
 */
#define OL_TICKS_PER_10MS   1   // 기구에 맞게 조정

/* ---- 초기 홈(센서 없음): 정해진 시간 동안 홈 방향으로 구동 ----
 * 센서가 없으므로 하드스톱을 직접 감지할 수 없다.
 * 따라서 타임아웃으로 동작을 종료하고, 그 시점을 POS_MIN(홈)으로 간주하여 저장한다.
 */
#define HOMING_TIMEOUT_MS   3000   // 축별 최대 홈 구동 시간(ms)
