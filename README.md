# Seat Relay Project

ESP32 + ULN2803 + 8채널 릴레이로 4축(슬라이드/앞/뒤/등받이)을 제어.

## 주요 구성
- RelayModule: 릴레이 ON/OFF 하드웨어 레이어
- RelayHBridge: 릴레이 2개로 안전한 정/역/정지(데드타임 포함)
- SeatNode: CAN 명령 처리 + 상태 리포트 + 초기 Homing + NVS 영속화

## 빌드
ESP-IDF 설치 후:
```bash
idf.py set-target esp32
idf.py build flash monitor
CAN 명령(요약)
0x40 AXIS_DRIVE: data[1]=axis(0..3), data[2]=dir(0/1/2)

0x41 AXIS_STOP : data[1]=axis

0x42 AXIS_GOTO : data[1]=axis, data[2]=posH, data[3]=posL

0x50 POSITION_REPORT 요청 → 0x320/0x321로 현재 위치 응답(옵션 구현 시)
