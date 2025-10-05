// app_main.cpp
// -----------------------------------------------------------------------------
// ESP-IDF 애플리케이션 엔트리 포인트.
// - CAN(TWAI) 초기화
// - SeatNode 초기화 (릴레이 8채널/4축 제어 로직)
// - 주기 태스크 생성: 수신 폴링, 제어 루프(10ms), 상태 리포트(20Hz)
// -----------------------------------------------------------------------------

extern "C" { void app_main(); }

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "can_node.hpp"   // CAN 래퍼 (TWAI)
#include "seat_node.hpp"  // 4축 제어 상위 로직
#include "config.h"       // 핀/채널/주기/데드타임 설정

// 전역 객체(단일 인스턴스)
// - 실제 하드웨어 리소스를 잡는 객체이므로 앱 수명과 함께 유지
static const char* TAG = "APP";
static CanNode  CAN;
static SeatNode SEAT;

// -----------------------------------------------------------------------------
// 태스크: CAN 수신 폴링
// - TWAI 드라이버의 수신 큐를 빠르게 확인하고, 프레임이 있으면 CanNode가
//   onFrame() → SeatNode::onCanCmd()로 디스패치한다.
// - 블로킹 없이 짧게 주기 지연(1ms)만 걸어 CPU 점유를 낮춘다.
// -----------------------------------------------------------------------------
static void task_can_rx(void*)
{
  while (true) {
    CAN.pollRx();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

// -----------------------------------------------------------------------------
// 태스크: 제어 루프 (CTRL_TICK_MS 주기, 기본 10ms)
// - RelayHBridge의 데드타임 상태머신을 진행
// - 초기 Homing(센서 없는 시간 기반) 수행
// - 평상시에는 target을 따라 오픈루프 추종
// - SeatNode::tick10ms() 내부에서 모두 처리
// -----------------------------------------------------------------------------
static void task_ctrl(void*)
{
  TickType_t last = xTaskGetTickCount();
  for (;;) {
    vTaskDelayUntil(&last, pdMS_TO_TICKS(CTRL_TICK_MS));
    SEAT.tick10ms();
  }
}

// -----------------------------------------------------------------------------
// 태스크: 상태 리포트 (REPORT_TICK_MS 주기, 기본 50ms ≒ 20Hz)
// - 간단한 상태 프레임을 CAN으로 송신 (예: 각 축 pos 일부)
// -----------------------------------------------------------------------------
static void task_report(void*)
{
  TickType_t last = xTaskGetTickCount();
  for (;;) {
    vTaskDelayUntil(&last, pdMS_TO_TICKS(REPORT_TICK_MS));
    SEAT.report20Hz();
  }
}

// -----------------------------------------------------------------------------
// app_main()
// - ESP-IDF가 부팅 후 호출하는 엔트리 포인트
// - 하드웨어/노드 초기화 후 3개의 태스크를 생성한다.
//   * task_can_rx : 수신 폴링
//   * task_ctrl   : 10ms 제어 루프
//   * task_report : 20Hz 상태 전송
// -----------------------------------------------------------------------------
void app_main()
{
  ESP_LOGI(TAG, "Seat Relay Project 부팅 시작");

  // 1) CAN(TWAI) 시작
  if (!CAN.begin()) {
    ESP_LOGE(TAG, "CAN 초기화 실패 - 재부팅 또는 설정 확인 필요");
    // 보수적으로 무한 대기(필요시 esp_restart() 등으로 대체 가능)
    while (true) { vTaskDelay(pdMS_TO_TICKS(1000)); }
  }

  // 2) SeatNode 시작 (릴레이/브리지/영속화/초기 Homing 포함)
  if (!SEAT.begin(&CAN)) {
    ESP_LOGE(TAG, "SeatNode 초기화 실패");
    while (true) { vTaskDelay(pdMS_TO_TICKS(1000)); }
  }

  // 3) 태스크 생성
  //    스택 크기/우선순위는 프로젝트 상황에 맞게 조정 가능
  xTaskCreate(task_can_rx, "can_rx", 4096, nullptr, 9, nullptr);
  xTaskCreate(task_ctrl,   "ctrl",   4096, nullptr, 8, nullptr);
  xTaskCreate(task_report, "report", 4096, nullptr, 5, nullptr);

  ESP_LOGI(TAG, "태스크 시작 완료");
}
