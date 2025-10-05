#include "seat_node.hpp"
#include "esp_log.h"
#include "state_store.hpp"

static const char* TAG = "Seat";

/**
 * CAN 명령 포맷 (Seat)
 * -------------------------------------------------------------------------
 * 공통: data[0] = CMD 코드
 *
 * 0x40 : AXIS_DRIVE  (축을 즉시 정/역/정지)
 *   data[0] = 0x40
 *   data[1] = axis  (0:SLIDE, 1:FRONT, 2:REAR, 3:RECLINE)   // 0..3
 *   data[2] = dir   (0:OFF/STOP, 1:FWD/UP, 2:REV/DOWN)
 *   data[3..7] = 예약(0)
 *   예) SLIDE 정방향 : 40 00 01 00 00 00 00 00
 *       REAR  정지   : 40 02 00 00 00 00 00 00
 *
 * 0x41 : AXIS_STOP   (AXIS_DRIVE에서 dir=0과 동일)
 *   data[0] = 0x41
 *   data[1] = axis  (0..3)
 *   data[2..7] = 0
 *   예) RECLINE 정지 : 41 03 00 00 00 00 00 00
 *
 * 0x42 : AXIS_GOTO   (오픈루프 목표 위치, 16비트 Big-endian)
 *   data[0] = 0x42
 *   data[1] = axis  (0..3)
 *   data[2] = posH
 *   data[3] = posL
 *   target = (posH<<8) | posL   // [POS_MIN..POS_MAX]로 clamp
 *   data[4..7] = 예약(0)
 *   예) FRONT 목표 800 : 42 01 03 20 00 00 00 00
 */

bool SeatNode::begin(CanNode* c){
  can_ = c;
  can_->onSeatCmd = [this](const CanMsg& m){ this->onCanCmd(m); };

  // 릴레이 8채널 초기화: HIGH=ON (ULN2803 입력은 active-high 권장)
  relays_.begin(RELAY_PINS, 8, /*active_high=*/true);

  // 축별로 두 릴레이를 안전한 H-브리지로 묶기(데드타임 적용)
  hb_[SLIDE  ].attach(&relays_, SLIDE_RLY_A,   SLIDE_RLY_B,   HBRIDGE_DEADTIME_MS);
  hb_[FRONT  ].attach(&relays_, FRONT_RLY_A,   FRONT_RLY_B,   HBRIDGE_DEADTIME_MS);
  hb_[REAR   ].attach(&relays_, REAR_RLY_A,    REAR_RLY_B,    HBRIDGE_DEADTIME_MS);
  hb_[RECLINE].attach(&relays_, RECL_RLY_A,    RECL_RLY_B,    HBRIDGE_DEADTIME_MS);

  // NVS에서 위치 복구(있으면 로드, 없으면 초기화 후 저장)
  store_.begin();
  if (store_.load(persist_)) {
    for (int i=0;i<AXIS_COUNT;i++) pos_[i] = persist_.pos[i];
    ESP_LOGI(TAG, "NVS에서 위치 로드: %d %d %d %d", pos_[0], pos_[1], pos_[2], pos_[3]);
  } else {
    for (int i=0;i<AXIS_COUNT;i++) { pos_[i]=0; persist_.pos[i]=0; persist_.speed_pct[i]=100; }
    store_.save(persist_);
    ESP_LOGI(TAG, "NVS 초기화: 모두 0으로 저장");
  }

  // 초기 홈 동작(센서 없음): 모든 축을 HOMING_TIMEOUT_MS 동안 홈 방향(FWD)으로 구동
  homing_active_ = true;
  homing_elapsed_ms_ = 0;
  for (int i=0;i<AXIS_COUNT;i++) hb_[i].request(HBState::FWD); // FWD가 '홈 쪽'이라고 가정

  ESP_LOGI(TAG, "SeatNode 준비완료 (RelayModule active_high=%d), 초기 홈 시작 (타임아웃=%d ms)",
           relays_.isActiveHigh(), HOMING_TIMEOUT_MS);
  return true;
}

void SeatNode::axisDrive(Axis ax, HBState s){
  if (ax<0 || ax>=AXIS_COUNT) return;
  hb_[ax].request(s);
}

void SeatNode::onCanCmd(const CanMsg& m){
  const uint8_t* d = m.data;
  switch(d[0]){
    case 0x40: { // AXIS_DRIVE
      Axis ax = (Axis) (d[1] % AXIS_COUNT);
      HBState st = HBState::OFF;
      if (d[2]==1) st = HBState::FWD;
      else if (d[2]==2) st = HBState::REV;
      axisDrive(ax, st);
    } break;

    case 0x41: { // AXIS_STOP
      Axis ax = (Axis) (d[1] % AXIS_COUNT);
      axisDrive(ax, HBState::OFF);
    } break;

    case 0x42: { // AXIS_GOTO (오픈루프 데모)
      Axis ax = (Axis) (d[1] % AXIS_COUNT);
      int tgt = (int)((d[2] << 8) | d[3]);
      target_[ax] = clamp(tgt, POS_MIN, POS_MAX);
    } break;

    default: break;
  }
}

void SeatNode::tick10ms(){
  // 상태 머신 진행(데드타임 포함)
  for (int i=0;i<AXIS_COUNT;i++) hb_[i].tick(CTRL_TICK_MS);

  // 센서 없는 초기 홈: HOMING_TIMEOUT_MS 동안 FWD로 이동 → 정지 후 POS_MIN으로 설정/저장
  if (homing_active_) {
    homing_elapsed_ms_ += CTRL_TICK_MS;

    // 홈 이동 중 간단한 오픈루프 카운터(예: FWD면 감소 방향으로 가정)
    for (int i=0;i<AXIS_COUNT;i++)
      if (hb_[i].state()==HBState::FWD) pos_[i] -= OL_TICKS_PER_10MS;

    if (homing_elapsed_ms_ >= HOMING_TIMEOUT_MS) {
      for (int i=0;i<AXIS_COUNT;i++) {
        hb_[i].request(HBState::OFF);
        pos_[i] = POS_MIN;
        persist_.pos[i] = pos_[i];
      }
      (void)store_.save(persist_);
      homing_active_ = false;
      ESP_LOGI(TAG, "초기 홈 완료: 모든 축 POS_MIN으로 설정 및 저장");
    }
    return; // 홈 동작 중에는 목표 추종을 하지 않음
  }

  // 오픈루프 목표 추종(데모): target에 따라 FWD/REV/OFF 지령 + pos 가상 적분
  for (int i=0;i<AXIS_COUNT;i++){
    int err = target_[i] - pos_[i];
    if      (err >  +POS_DEADBAND) { hb_[i].request(HBState::FWD);  pos_[i] += OL_TICKS_PER_10MS; }
    else if (err <  -POS_DEADBAND) { hb_[i].request(HBState::REV);  pos_[i] -= OL_TICKS_PER_10MS; }
    else                           { hb_[i].request(HBState::OFF); }
  }

  // 주기적 위치 저장(약 1초마다)
  static uint32_t acc_ms = 0;
  acc_ms += CTRL_TICK_MS;
  if (acc_ms >= 1000) {
    acc_ms = 0;
    for (int i=0;i<AXIS_COUNT;i++) persist_.pos[i] = pos_[i];
    (void)store_.save(persist_);
    ESP_LOGI(TAG, "주기 저장: pos = %d %d %d %d", pos_[0], pos_[1], pos_[2], pos_[3]);
  }
}

void SeatNode::report20Hz(){
  // 상태 프레임 예시: 앞 3개 축의 pos를 2바이트씩 담아 전송
  uint8_t d[8] = {0};
  for (int i=0;i<3 && i<AXIS_COUNT;i++){
    int p = clamp(pos_[i], POS_MIN, POS_MAX);
    d[2*i]   = (uint8_t)((p >> 8) & 0xFF);
    d[2*i+1] = (uint8_t)(p & 0xFF);
  }
  can_->sendState(0x310, d);
}
