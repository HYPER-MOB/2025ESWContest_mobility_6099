# HYPERMOB Platform TCU

**Telematics Control Unit** - CAN + REST API 통합 프로그램

## 개요

TCU는 차량의 CAN 버스와 인터넷(REST API)을 연결하는 게이트웨이입니다.

- **CAN 통신**: Library-CAN을 사용하여 SCA/DCU와 통신
- **REST API**: Platform-Application 서버와 HTTP/HTTPS 통신
- **통합 구조**: Library-CAN이 Platform-TCU 내부에 포함됨

## 프로젝트 구조

```
Platform-TCU/
├── Library-CAN/              # CAN 통신 라이브러리 (복사본)
│   ├── can_api.h/c
│   ├── canmessage.h/c
│   ├── channel.h/c
│   ├── adapter*.c
│   └── ...
├── include/
│   └── tcu_rest_client.h     # REST API 클라이언트 헤더
├── src/
│   ├── tcu_main.c            # 메인 프로그램 (300줄)
│   └── tcu_rest_client.c     # REST API 구현
├── config/
│   └── tcu_config.json
└── CMakeLists.txt            # 빌드 설정
```

## 빌드

### Linux/Raspberry Pi

```bash
# 의존성 설치
sudo apt-get install -y \
    build-essential cmake \
    libcurl4-openssl-dev \
    libcjson-dev \
    libsocketcan-dev \
    can-utils

# 빌드
cd Platform-TCU
mkdir build && cd build
cmake ..
make -j$(nproc)

# 설치 (선택사항)
sudo make install
```

### Windows (MinGW/MSYS2)

```bash
# MSYS2에서 의존성 설치
pacman -S mingw-w64-x86_64-gcc \
          mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-curl \
          mingw-w64-x86_64-cjson

# 빌드
cd Platform-TCU
mkdir build && cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
```

### 직접 빌드 (Linux)

```bash
gcc -O2 -Wall -D_CRT_SECURE_NO_WARNINGS \
    src/tcu_main.c \
    src/tcu_rest_client.c \
    Library-CAN/can_api.c \
    Library-CAN/canmessage.c \
    Library-CAN/channel.c \
    Library-CAN/adapterfactory.c \
    Library-CAN/adapter_linux.c \
    -ILibrary-CAN \
    -Iinclude \
    -lcurl -lcjson -lsocketcan -lpthread -lm \
    -o hypermob-tcu
```

## 실행

### 기본 실행

```bash
# Linux
sudo ./hypermob-tcu can0 https://your-api.com/v1 CAR123

# 인자 없이 실행 (기본값 사용)
sudo ./hypermob-tcu
```

### 파라미터

```bash
# 형식: ./hypermob-tcu [CAN_INTERFACE] [API_URL] [CAR_ID]
# 예시:
sudo ./hypermob-tcu can0 https://api.hypermob.com/v1 CAR001
```

### 환경 변수

```bash
export HYPERMOB_APP_API_URL="https://your-api-id.execute-api.us-east-1.amazonaws.com/v1"
export HYPERMOB_CAR_ID="CAR123"
```

### 사용자 설정

```bash
# 현재 인증 사용자 변경
echo "USER123" > /tmp/tcu_user

# 확인
cat /tmp/tcu_user
```

## 시스템 플로우

### seq_diagram.md 기준 인증/프로필 플로우

```
1. SCA → TCU (CAN 0x102): 사용자 인증 정보 요청
   └─> TCU → Server: GET /auth/session
   └─> TCU → SCA (CAN 0x107): NFC 정보 전송
   └─> TCU → Server: POST /auth/result

2. DCU → TCU (CAN 0x201): 사용자 프로필 요청
   └─> TCU → Server: GET /users/{user_id}/profile
   └─> TCU → DCU (CAN 0x202/0x203/0x204): 시트/미러/휠 프로필 전송

3. DCU → TCU (CAN 0x206/0x207/0x208): 수동 조정 보고
   └─> TCU → Server: POST /vehicles/{car_id}/settings/manual
   └─> TCU → DCU (CAN 0x209): ACK 전송
```

## 주요 CAN 메시지

### TCU가 수신하는 메시지

| ID    | 이름                           | 발신자    | 설명                    |
|-------|--------------------------------|-----------|-------------------------|
| 0x102 | SCA_TCU_USER_INFO_REQ          | SCA       | 사용자 인증 정보 요청   |
| 0x201 | DCU_TCU_USER_PROFILE_REQ       | DCU       | 사용자 프로필 요청      |
| 0x206 | DCU_TCU_USER_PROFILE_SEAT_UPDATE | DCU     | 시트 수동 조정 보고     |
| 0x207 | DCU_TCU_USER_PROFILE_MIRROR_UPDATE | DCU   | 미러 수동 조정 보고     |
| 0x208 | DCU_TCU_USER_PROFILE_WHEEL_UPDATE | DCU    | 휠 수동 조정 보고       |

### TCU가 전송하는 메시지

| ID    | 이름                           | 수신자    | 설명                    |
|-------|--------------------------------|-----------|-------------------------|
| 0x107 | TCU_SCA_USER_INFO_NFC          | SCA       | NFC UID 전송 (8 bytes)  |
| 0x202 | TCU_DCU_USER_PROFILE_SEAT      | DCU       | 시트 프로필 전송        |
| 0x203 | TCU_DCU_USER_PROFILE_MIRROR    | DCU       | 미러 프로필 전송        |
| 0x204 | TCU_DCU_USER_PROFILE_WHEEL     | DCU       | 휠 프로필 전송          |
| 0x209 | TCU_DCU_USER_PROFILE_UPDATE_ACK | DCU      | 프로필 업데이트 ACK     |

## REST API 엔드포인트

### Platform-Application APIs

**인증:**
- `GET /auth/session?car_id={car_id}&user_id={user_id}` - 인증 세션 요청
- `POST /auth/result` - 인증 결과 보고 (MFA)
- `GET /health` - API 상태 확인

**사용자 프로필:**
- `GET /users/{user_id}/profile` - 사용자 프로필 조회
- `PUT /users/{user_id}/profile` - 사용자 프로필 업데이트

**차량 설정:**
- `POST /vehicles/{car_id}/settings/apply` - 자동 설정 적용
- `POST /vehicles/{car_id}/settings/manual` - 수동 조정 기록
- `GET /vehicles/{car_id}/settings/current` - 현재 설정 조회

자세한 내용: [Platform-Application/openapi.yaml](../Platform-Application/.docs/openapi.yaml)

## 테스트

### CAN 시뮬레이션 (Linux)

```bash
# Terminal 1: TCU 실행
sudo ./hypermob-tcu

# Terminal 2: CAN 모니터
candump can0

# Terminal 3: 테스트 메시지 전송
# SCA가 사용자 정보 요청
cansend can0 102#01

# DCU가 프로필 요청
cansend can0 201#01

# DCU가 시트 조정 보고 (pos=50, angle=90, fh=60, rh=55)
cansend can0 206#32.5A.3C.37
```

### 로그 확인

```bash
# TCU 로그
./hypermob-tcu 2>&1 | tee tcu.log

# CAN 트래픽 로그
candump can0 -L
```

## 트러블슈팅

### CAN 인터페이스 설정 (Linux)

```bash
# CAN 인터페이스 확인
ip link show can0

# CAN 활성화
sudo ip link set can0 type can bitrate 500000
sudo ip link set can0 up

# 상태 확인
ip -details link show can0
```

### 빌드 에러

**cJSON not found:**
```bash
sudo apt-get install libcjson-dev
```

**libsocketcan not found:**
```bash
sudo apt-get install libsocketcan-dev
```

**CURL not found:**
```bash
sudo apt-get install libcurl4-openssl-dev
```

### 권한 에러

```bash
# CAN 접근 권한 필요
sudo ./hypermob-tcu

# 또는 사용자 권한 추가
sudo usermod -aG dialout $USER
sudo usermod -aG plugdev $USER
# 재로그인 필요
```

## 코드 구조

### tcu_main.c 핵심 로직

```c
// Library-CAN 사용 예시

// 1. CAN 메시지 전송 (NFC)
CanMessage msg = {0};
msg.tcu_sca_user_info_nfc.sig_user_nfc = nfc_value;
CanFrame frame = can_encode_pcan(PCAN_ID_TCU_SCA_USER_INFO_NFC, &msg, 8);
can_send(g_can_interface, frame, 1000);

// 2. 시트 프로필 전송
msg.tcu_dcu_user_profile_seat.sig_seat_position = seat_pos;
msg.tcu_dcu_user_profile_seat.sig_seat_angle = seat_angle;
frame = can_encode_pcan(PCAN_ID_TCU_DCU_USER_PROFILE_SEAT, &msg, 4);
can_send(g_can_interface, frame, 1000);

// 3. CAN 수신 콜백 등록
CanFilter filter = {.type = CAN_FILTER_MASK};
filter.data.mask.id = 0x102;
filter.data.mask.mask = 0x7FF;
can_subscribe(g_can_interface, &sub_id, filter, on_sca_user_info_req, NULL);
```

### tcu_rest_client.c 핵심 로직

```c
// REST API 호출 예시

// 1. 인증 세션 요청
TcuAuthSession session;
tcu_rest_get_auth_session(car_id, user_id, &session);

// 2. 사용자 프로필 조회
TcuUserProfile profile;
tcu_rest_get_user_profile(user_id, &profile);

// 3. 수동 조정 기록
tcu_rest_record_manual_settings(car_id, user_id, &settings);
```

## 배포

### Systemd 서비스 (Linux)

`/etc/systemd/system/hypermob-tcu.service`:

```ini
[Unit]
Description=HYPERMOB Platform TCU
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/opt/hypermob/Platform-TCU
Environment="HYPERMOB_APP_API_URL=https://your-api.com/v1"
Environment="HYPERMOB_CAR_ID=CAR123"
ExecStart=/usr/local/bin/hypermob-tcu can0 https://your-api.com/v1 CAR123
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable hypermob-tcu
sudo systemctl start hypermob-tcu
sudo systemctl status hypermob-tcu

# 로그 확인
sudo journalctl -u hypermob-tcu -f
```

## 라이선스

Copyright (c) 2025 HYPERMOB Platform Team

## 관련 문서

- [Library-CAN README](Library-CAN/README.md) - CAN 라이브러리 문서
- [CAN DB](../CAN%20DB%20-%20Private%20CAN.csv) - CAN 메시지 정의
- [Sequence Diagram](../seq_diagram.md) - 시스템 시퀀스 다이어그램
- [Platform-Application API](../Platform-Application/.docs/openapi.yaml) - REST API 명세
