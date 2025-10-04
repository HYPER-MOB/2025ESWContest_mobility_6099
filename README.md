# Library-CAN

플랫폼 독립적인 CAN 통신 라이브러리  

- **상위 API (`can_api`)** 만 호출하면 동작
- 하드웨어별/운영체제별 처리는 **Adapter**가 캡슐화
- **자동 Bring-up 지원**: Linux 환경에서는 `ip link set …` 없이 코드에서 인터페이스 활성화
- Linux(SocketCAN) / ESP32(TWAI) / Debug(PC) 환경 모두 지원

---

## 📂 프로젝트 구조

```
.
├── adapter.h                   # Adapter 인터페이스
├── adapter_linux.c             # Linux(SocketCAN) 어댑터 (libsocketcan 기반)
├── adapter_esp32.c             # ESP32 TWAI 어댑터
├── adapterfactory.c            # create_adapter() 구현
├── can_api.h / can_api.c       # 공용 API (사용자가 호출)
├── channel.h / channel.c       # 채널, 구독/Job 관리
├── canmessage.h / canmessage.c # 메시지 정의/인코딩/디코딩
└── README.md
```

---

## 🚀 빠른 시작

### 공용 API

```c

#include "can_api.h"
#include "canmessage.h"

void seq_producer(CanFrame* f, void* u) { 
    static uint8_t s; 
    f->id=0x321; 
    f->dlc=2; 
    f->data[0]=0xAA; 
    f->data[1]=s++; 
}

void on_rx_single(const CanFrame* fr, void* user) {
    printf("[RX] single id=0x%03X dlc=%u\n", fr->id, fr->dlc);
}

void on_rx_range(const CanFrame* fr, void* user) {
    printf("[RX] range id=0x%03X dlc=%d\n", fr->id, fr->dlc);
}

can_init(CAN_DEVICE_LINUX); // 또는 CAN_DEVICE_ESP32

CanConfig cfg = {
  .channel     = 0,
  .bitrate     = 500000,
  .samplePoint = 0.875f,
  .sjw         = 1,
  .mode        = CAN_MODE_NORMAL
};

can_open("can0", cfg);             // Linux: adapter가 자동 bring-up

CanFrame fr = {.id=0x123, .dlc=3, .data={0x11,0x22,0x33}};
if(can_send("can0", fr, 1000) == CAN_OK) {}                       // 단일 메시지 쓰기 (timeout은 ms단위)

CanFrame fr = {0};
if(can_recv("can0", &fr, 1000) == CAN_OK) {}                      // 단일 메시지 읽어오기 (timeout 시 CAN_ERR_TIMEOUT)

int subID_range     = 0;
CanFilter f_any     = {.type = CAN_FILTER_MASK,   .data.mask={.id=0, .mask=0}};                   // 모든 메시지를 통과하는 필터
if(can_subscribe("can0", &subID_any, f_any, on_rx_single, NULL) == CAN_OK) {}                     // 모든 메시지에 대해서 콜백 등록

int subID_single    = 0;
CanFilter f_single  = {.type = CAN_FILTER_MASK,   .data.mask = { .id = 0x001, .mask = 0x7FF }};   // 정확히 0x001만 허용
if(can_subscribe("can0", &subID_single, f_single, on_rx_single, NULL) == CAN_OK) {}               // 단일 메시지에 대해서 콜백 등록

int subID_any       = 0;
CanFilter f_range   = {.type = CAN_FILTER_RANGE,  .data.range = { .min = 0x001, .max = 0x004 }};  // 0x001 ~ 0x004 허용
if(can_subscribe("can0", &subID_range, f_range, on_rx_range, NULL) == CAN_OK) {}                  // 범위 메시지에 대해서 콜백 등록

int jobID = 0;
CanFrame fr = {.id=0x123, .dlc=3, .data={0x11,0x22,0x33}};
if(can_register_job("can0", &jobID, fr, 100) == CAN_OK) {}                                       // 100ms 주기 송신 (고정된 값)

int jobEXID = 0;
if(can_register_job_ex("can0", &jobEXID, NULL, 200, seq_producer, NULL) == CAN_OK) {}             // 200ms 주기 송신, 송신할 때 seq_producer를 호출하여 frame의 값을 결정

CanMessage msg = {0};
msg.dcu_wheel_order = {.sig_wheel_position = 10, .sig_wheel_angle = 20};
CanFrame frame = can_encode_bcan(BCAN_ID_DCU_SEAT_ORDER, &msg, 2);                                 // CanMessage를 이용해 frame build

can_msg_pcan_id_t id;
CanMessage msg = {0};
CanFrame frame = {0};
can_recv("can0", &frame, 1000);                                 // 혹은 다른 방식으로 frame을 받아왔을 때
if(can_decode_bcan(frame, &id, &msg) == CAN_OK) {
  if(id == BCAN_ID_DCU_WHEEL_ORDER) {
    printf("id=0x%03X wheel pos=%d wheel angle = %d\n", msg.dcu_wheel_order.sig_wheel_position, msg.dcu_wheel_order.sig_wheel_angle);
  }
}

if(can_unsubscribe("can0", subID_any) == CAN_OK) {}             // 모든 메시지 콜백 등록 해제
if(can_unsubscribe("can0", subID_single) == CAN_OK) {}          // 단일 메시지 콜백 등록 해제
if(can_unsubscribe("can0", subID_range) == CAN_OK) {}           // 범위 메시지 콜백 등록 해제

if(can_cancel_job("can0", jobID) == CAN_OK) {}                  // 고정 메시지 주기 송신 작업 해제
if(can_cancel_job("can0", jobEXID) == CAN_OK) {}                // 변동 메시지 주기 송신 작업 해제

can_close("can0");
can_dispose();

```

---

## 🛠️ 플랫폼별 설정

### 1. Raspberry Pi (MCP2515 + TJA1050)

- `/boot/config.txt` 수정:
  ```ini
  dtparam=spi=on
  dtoverlay=mcp2515-can0,oscillator=8000000,interrupt=25
  ```
- 재부팅 후:
  ```bash
  dmesg | grep mcp2515
  ip link show
  ```
- 이후에는 `adapter_linux` 가 자동으로 bring-up 처리.

---

### 2. Jetson Nano (MCP2515)

- 라즈베리파이와 원리 동일
- `/boot/extlinux/extlinux.conf` 에 DT overlay 적용
- `can0` 인터페이스가 노출되면 이후는 라이브러리가 자동 처리

---

### 3. ESP32-WROOM-32 (TWAI)

- ESP32는 내장 CAN(TWAI) 컨트롤러 사용 → 트랜시버(SN65HVD230 등)만 연결
- `adapter_esp32.c` 가 Arduino/IDF 환경에서 동작
- 예제:

```cpp
extern "C" { #include "can_api.h" }
void setup() {
  Serial.begin(115200);
  can_init(CAN_DEVICE_ESP32);

  CanConfig cfg = {.channel=0, .bitrate=500000, .samplePoint=0.875f, .sjw=1, .mode=CAN_MODE_NORMAL};
  can_open("can0", cfg);

  Serial.println("ESP32 CAN ready");
}
void loop() { delay(1000); }
```

---

## 🧰 빌드

### Linux
```bash
sudo apt install -y build-essential pkg-config libsocketcan-dev can-utils

gcc -O2 -Wall main.c adapterfactory.c adapter_linux.c can_api.c canmessage.c channel.c -lsocketcan -lpthread -o can_job_test

# main.c는 각자 작성한 소스 코드

sudo ./can_job_test
# 또는
sudo setcap cap_net_admin+ep ./can_job_test
./can_job_test
```

### ESP32 (Arduino IDE)

- `.ino` 또는 `.c` 스케치에 라이브러리 파일 포함
- TWAI RX/TX 핀을 SN65HVD230/1050 트랜시버와 연결

---

## 📡 배선 요약

- CAN_H ↔ CAN_H, CAN_L ↔ CAN_L, GND 공통 연결
- 종단저항 120Ω 양쪽 끝 필수
- 트랜시버 **RS/EN/STB → LOW** (노멀 모드)
- Raspberry Pi SPI0.0:
  - MOSI GPIO10(핀19), MISO GPIO9(핀21), SCLK GPIO11(핀23), CS0 GPIO8(핀24), INT GPIO25(핀22)

---

## 🔎 트러블슈팅

- 인터페이스는 보이나 통신 불가 → 비트레이트 불일치, 배선 오류, 종단저항 문제
- 샘플포인트 불일치 → MCP2515 드라이버는 근사치 강제 적용
- bring-up 실패 → root 권한 또는 `setcap cap_net_admin+ep` 필요

---

## 📜 라이선스

- SocketCAN (Linux kernel)
- ESP-IDF TWAI driver
- MIT License (추후 결정 가능)
