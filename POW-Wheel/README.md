# 🛞 POW-Wheel — ESP32 스티어링 휠 제어 시스템 (릴레이 · CAN · NVS)

## 🔍 프로젝트 개요

**POW-Wheel**은 ESP32를 사용하여 **스티어링 휠 위치 및 각도 제어**를 수행하는 시스템이다.
CAN(TWAI) 네트워크를 통해 DCU와 통신하며, **NotReady → Ready → Busy** 상태를 기반으로 동작하는 FSM 구조를 사용한다.
Homing 시퀀스 수행 후 DCU로 **리셋 ACK** 신호를 전송한다.

---

## 📁 프로젝트 구조

```
POW-Wheel/
├── POW-Wheel.ino            # 메인 스케치 (ESP32용, 릴레이/NVS/FSM/CAN glue)
├── adapter.h                # CAN 어댑터 추상화 인터페이스
├── adapter_esp32.c          # ESP32 TWAI 드라이버 기반 어댑터 구현
├── adapterfactory.c         # 어댑터 생성기 (ESP32 / Linux 등 선택)
├── can_api.c                # CAN API 레이어 (채널 관리 및 메시지 송수신)
├── can_api.h                # CAN API 정의
├── canmessage.c             # CAN 프레임 인코딩/디코딩 구현
├── canmessage.h             # CAN 메시지 정의 (신호/ID/DLC 매핑)
├── channel.c                # 채널 객체 관리 (콜백, 구독, Job 등)
└── channel.h                # 채널 인터페이스 정의
```

* **CAN API/채널 레이어**: `can_init`, `can_open`, `can_send`, `can_subscribe`, `can_register_job_dynamic` 등을 제공하며, 채널 콜백/필터/잡 관리가 포함된다.
* **ESP32 어댑터**: ESP-IDF TWAI를 래핑하여 송·수신, **동적 Job**/취소, 상태/복구, 콜백 설정을 구현한다.
* **메시지 정의**: DBC에 준한 BCAN/PCAN **ID·DLC·페이로드** 구조와 encode/decode 유틸이 제공된다.

---

## ⚙️ 빌드 & 플래싱

1. Arduino IDE 또는 VSCode + PlatformIO 사용
2. 보드: **ESP32 Dev Module**
3. 시리얼 속도: **115200 baud**
4. 업로드 후 자동 실행

---

## 🔌 하드웨어 핀

| 구분             | Pin + | Pin - |
| -------------- | -- | -- |
| Wheel Position | 13 | 14 |
| Wheel Angle    | 26 | 27 |

---

## 🧠 FSM 상태

| 상태           | 설명                           |
| ------------ | ---------------------------- |
| **NotReady** | Homing 수행 및 DCU_RESET_ACK 전송 |
| **Ready**    | CAN 명령 및 버튼 입력 대기            |
| **Busy**     | 목표 위치 이동 중, 완료 시 Ready 전이    |

---

## 🛰️ CAN 요약

| 항목    | 값                                            |
| ----- | -------------------------------------------- |
| 채널명   | `"twai0"`                                    |
| 비트레이트 | 500 kbps                                     |
| 송신 주기 | 20 ms                                        |
| 구독 ID | DCU_RESET, DCU_WHEEL_ORDER, DCU_WHEEL_BUTTON |
| 송신 ID | DCU_RESET_ACK, POW_WHEEL_STATE               |

---

## 🧩 메시지 맵

| 구분           | ID                       | 방향 | 설명                  |
| ------------ | ------------------------ | -- | ------------------- |
| Reset        | BCAN_ID_DCU_RESET        | RX | 리셋 요청 수신            |
| Reset Ack    | BCAN_ID_DCU_RESET_ACK    | TX | Homing 완료 후 응답      |
| Wheel Order  | BCAN_ID_DCU_WHEEL_ORDER  | RX | 목표 각도 명령            |
| Wheel Button | BCAN_ID_DCU_WHEEL_BUTTON | RX | 버튼 조작 입력            |
| Wheel State  | BCAN_ID_POW_WHEEL_STATE  | TX | 현재 상태 주기 전송 (20 ms) |

---

## 💾 NVS(플래시) 저장

| 항목       | 값                    |
| -------- | -------------------- |
| 네임스페이스   | `"wheel"`            |
| 저장 키     | pos, ang |
| 스키마 버전   | 1                    |
| 최소 저장 간격 | 2초                   |

---

## 💻 시리얼 명령

| 명령             | 설명                |
| -------------- | ----------------- |
| `status`       | FSM 상태 및 현재 위치 표시 |
| `reset status` | 휠 동작 중지 및 상태 초기화  |

---

## 🛡️ 안전 노트

* 방향 전환 시 **60 ms PreHold** 적용
* Jog 해제 시 즉시 정지
* 릴레이 구동 간격 최소 80 ms 확보
* 플래시 저장 최소 주기 2초

---

## 📄 라이선스

MIT License

---
