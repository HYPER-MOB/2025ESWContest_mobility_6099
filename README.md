# HYPERMOB - 초개인화 주행 플랫폼

> **2025 임베디드 소프트웨어 경진대회 - Mobility 부문 6099팀**

HYPERMOB는 바디스캔 기술과 다중 인증(MFA)을 결합하여 개인 맞춤형 주행 경험을 제공하는 차세대 차량 공유 플랫폼입니다.

---

## 프로젝트 개요

이 저장소는 HYPERMOB 플랫폼의 모든 서브 시스템을 git subtree를 통해 통합 관리하는 monorepo입니다. 각 서브 시스템은 독립적으로 개발되며, 이 저장소에서 전체 시스템을 통합하여 관리합니다.

### 핵심 기능

- **Multi-Factor Authentication (MFA)**: 얼굴 인식 + NFC + BLE 기반 3단계 다중 인증
- **바디스캔**: MediaPipe 기반 신체 측정 및 AI 프로필 생성
- **자동 차량 세팅**: AI 기반 개인 맞춤형 시트/미러/핸들 설정 (30초 이내)
- **졸음 감지 시스템**: 실시간 운전자 상태 모니터링 및 경보
- **수동 미세 조정**: DCU-FE 화면에서 시트/미러/핸들 실시간 제어

---

## 프로젝트 목표 및 특징

### 개발 배경

기존 차량 공유 서비스는 다음과 같은 문제점을 가지고 있습니다:
- **보안 문제**: 단순한 PIN 코드나 스마트키 방식으로는 차량 도용 위험이 높음
- **불편한 사용자 경험**: 매번 시트, 미러, 핸들 등을 수동으로 조정해야 함
- **안전성 문제**: 운전자의 피로도나 졸음 상태를 실시간으로 감지하지 못함

### 프로젝트 목표

HYPERMOB는 이러한 문제를 해결하기 위해 다음과 같은 목표를 설정했습니다:

1. **강력한 보안**: NFC + BLE + 얼굴 인식을 결합한 3단계 MFA로 차량 도용 방지
2. **개인화된 주행 경험**: 신체 치수 기반 AI 추천으로 최적의 차량 환경 자동 설정
3. **안전 운행**: 실시간 졸음 감지로 사고 예방 및 운전자 안전 확보
4. **편리한 사용성**: 모바일 앱 기반 원스톱 차량 공유 서비스

### 차별점

| 특징 | 기존 서비스 | HYPERMOB |
|------|------------|----------|
| **인증 방식** | 단일 인증 (PIN/앱) | 3단계 MFA (NFC+BLE+얼굴) |
| **차량 설정** | 수동 조정 필요 | AI 기반 자동 설정 |
| **신체 측정** | 수동 입력 | 바디스캔 자동 측정 |
| **안전 기능** | 없음 | 실시간 졸음 감지 |
| **프로필 저장** | 로컬 저장 | 클라우드 기반 어디서나 접근 |
| **적용 시간** | 3~5분 | 30초 이내 |

---

## 시스템 아키텍처

```
┌─────────────────────────────────────────────────────────────────┐
│                        모바일 애플리케이션                           │
│                    (Android - Kotlin + Compose)                  │
└────────────────────────┬────────────────────────────────────────┘
                         │ REST API (HTTPS)
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                        Platform Server                           │
│              (AWS Lambda + API Gateway + RDS)                    │
│  ┌──────────────────┐  ┌──────────────────┐  ┌───────────────┐ │
│  │  Node.js Lambda  │  │  Python Lambda   │  │   MySQL DB    │ │
│  │  (Business Logic)│  │  (AI Services)   │  │               │ │
│  └──────────────────┘  └──────────────────┘  └───────────────┘ │
└────────────────────────┬────────────────────────────────────────┘
                         │ REST API (HTTPS)
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                    TCU (Telematics Control Unit)                 │
│                         (Raspberry Pi)                           │
│              CAN ↔ REST API Gateway                             │
└────────┬───────────────────────────────────────────────┬────────┘
         │ Private CAN (500kbps)                         │
         ▼                                               ▼
┌─────────────────────┐                        ┌──────────────────┐
│   SCA (Smart Car    │                        │  DCU (Driver     │
│    Access)          │                        │  Control Unit)   │
│  ┌──────────────┐   │                        │  ┌────────────┐  │
│  │  SCA-Core    │   │◄─── Private CAN ──────►│  │ DCU-Core   │  │
│  │  (Auth/CAN)  │   │                        │  │ (Backend)  │  │
│  └──────────────┘   │                        │  └────────────┘  │
│  ┌──────────────┐   │                        │  ┌────────────┐  │
│  │   SCA-AI     │   │                        │  │  DCU-FE    │  │
│  │ (Face/Drowsy)│   │                        │  │ (Frontend) │  │
│  └──────────────┘   │                        │  └────────────┘  │
│  (Raspberry Pi 4)   │                        │  (Raspberry Pi)  │
└─────────────────────┘                        └────────┬─────────┘
                                                        │
                                           Body CAN (500kbps)
                                                        │
                    ┌───────────────┬──────────────────┼─────────────────┐
                    ▼               ▼                  ▼                 ▼
            ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
            │  POW-Seat    │ │ POW-Mirror   │ │  POW-Wheel   │ │    (etc)     │
            │  (ESP32)     │ │  (ESP32)     │ │  (ESP32)     │ │              │
            └──────────────┘ └──────────────┘ └──────────────┘ └──────────────┘
```

---

## 프로젝트 구조

```
2025ESWContest_mobility_6099/
├── Platform-Application/    # Android 모바일 애플리케이션
├── Platform-Server/          # AWS Lambda 기반 백엔드 서버
├── Platform-AI/              # AI 서비스 (신체 측정, 차량 설정 추천)
├── TCU-Core/                 # CAN ↔ REST API 게이트웨이
├── DCU-Core/                 # 운전석 제어 유닛 백엔드
├── DCU-FE/                   # 운전석 제어 유닛 UI
├── SCA-Core/                 # 스마트 카 액세스 코어 (인증)
├── SCA-AI/                   # 얼굴 인증 및 졸음 감지 AI
├── POW-Seat/                 # 시트 제어 시스템 (ESP32)
├── POW-Mirror/               # 미러 제어 시스템 (ESP32)
├── POW-Wheel/                # 핸들 제어 시스템 (ESP32)
├── Library-CAN_ESP32/        # ESP32용 CAN 통신 라이브러리
├── Library-CAN_Linux/        # Linux용 CAN 통신 라이브러리
├── .subtree/                 # Subtree 설정 파일
└── setup_subtree.sh          # Subtree 관리 스크립트
```

---

## 주요 컴포넌트 설명

### 1. Platform Layer

#### Platform-Application
- **역할**: 사용자용 Android 모바일 앱
- **기술 스택**: Kotlin, Jetpack Compose, Clean Architecture
- **주요 기능**:
  - 사용자 등록 (얼굴 촬영, 신체 측정)
  - MFA 디바이스 등록 (NFC/BLE)
  - 바디스캔 (ML Kit 기반 신체 측정)
  - 차량 제어 UI (GUI 구현, 실제 제어는 DCU-FE에서 수행)
- **상세 문서**: [Platform-Application/README.md](./Platform-Application/README.md)

#### Platform-Server
- **역할**: 서버리스 백엔드 API
- **기술 스택**: AWS Lambda (Node.js/Python), API Gateway, RDS MySQL
- **주요 기능**:
  - 얼굴 이미지 저장 (S3) 및 랜드마크 추출
  - 신체 이미지 저장 및 치수 측정
  - NFC/BLE 인증 정보 관리
  - 사용자 프로필 저장 및 조회
  - MFA 인증 세션 관리
- **상세 문서**: [Platform-Server/README.md](./Platform-Server/README.md)

#### Platform-AI
- **역할**: AI 기반 신체 측정 및 추천 서비스
- **기술 스택**: Python, MediaPipe, scikit-learn, AWS Lambda
- **주요 기능**:
  - 신체 치수 측정 (팔/다리/몸통 길이)
  - 차량 설정 추천 (Ridge 회귀)
  - 얼굴 랜드마크 추출 (468개 특징점)
- **상세 문서**: [Platform-AI/README.md](./Platform-AI/README.md)

### 2. TCU (Telematics Control Unit)

#### TCU-Core
- **역할**: CAN 버스와 인터넷(REST API) 연결 게이트웨이
- **기술 스택**: C, libcurl, cJSON, Library-CAN
- **주요 기능**:
  - SCA/DCU와 CAN 통신
  - Platform-Server와 REST API 통신
  - 인증 정보 중계
  - 사용자 프로필 전송
- **상세 문서**: [TCU-Core/README.md](./TCU-Core/README.md)

### 3. DCU (Driver Control Unit)

#### DCU-Core
- **역할**: CAN 통신 백엔드 데몬
- **기술 스택**: C++17, Qt (Core/Network), Library-CAN
- **주요 기능**:
  - Body CAN 버스 제어
  - IPC 기반 UI 통신 (JSON)
  - POW 장치 상태 관리
- **상세 문서**: [DCU-Core/README.md](./DCU-Core/README.md)

#### DCU-FE
- **역할**: 운전석 제어 UI
- **기술 스택**: C++17, Qt 5.15+ (Widgets/Multimedia)
- **주요 기능**:
  - 인증 프로세스 표시
  - 차량 장치 상태 표시
  - 시트/미러/핸들 수동 제어
  - 졸음 경보 표시
- **상세 문서**: [DCU-FE/README.md](./DCU-FE/README.md)

### 4. SCA (Smart Car Access)

#### SCA-Core
- **역할**: 통합 인증 및 제어 코어
- **기술 스택**: C++, BlueZ, libnfc, Library-CAN
- **하드웨어**: Raspberry Pi 4, MCP2515, NFC(I2C), BLE
- **주요 기능**:
  - NFC 태그 인증 (ISO14443A)
  - BLE Peripheral 인증
  - 카메라 기반 얼굴 인증 연동
  - 주행 중 졸음 감지 신호 수신
- **상세 문서**: [SCA-Core/README.md](./SCA-Core/README.md)

#### SCA-AI
- **역할**: 얼굴 인증 및 졸음 감지 AI
- **기술 스택**: Python 3.12, MediaPipe, OpenCV, NumPy
- **주요 기능**:
  - 얼굴 랜드마크 기반 인증 (코사인 유사도)
  - Liveness 검사 (입 벌림/움직임)
  - 실시간 졸음 감지 (EAR/MAR/머리 기울기)
- **상세 문서**: [SCA-AI/README.md](./SCA-AI/README.md)

### 5. POW (Power-controlled Devices)

#### POW-Seat
- **역할**: 시트 제어 시스템
- **기술 스택**: Arduino (ESP32), Library-CAN
- **하드웨어**: ESP32, 8채널 릴레이, 착좌 센서
- **제어 항목**: 시트 전후/등받이 각도/전방 높이/후방 높이
- **상세 문서**: [POW-Seat/README.md](./POW-Seat/README.md)

#### POW-Mirror
- **역할**: 미러 제어 시스템
- **기술 스택**: Arduino (ESP32), Library-CAN
- **하드웨어**: ESP32, 릴레이, PCA9685 서보 드라이버
- **제어 항목**: 좌/우 사이드 미러 (Yaw/Pitch), 룸 미러 (Yaw/Pitch)
- **상세 문서**: [POW-Mirror/README.md](./POW-Mirror/README.md)

#### POW-Wheel
- **역할**: 스티어링 휠 제어 시스템
- **기술 스택**: Arduino (ESP32), Library-CAN
- **하드웨어**: ESP32, 릴레이
- **제어 항목**: 핸들 전후 위치, 핸들 각도
- **상세 문서**: [POW-Wheel/README.md](./POW-Wheel/README.md)

### 6. Library

#### Library-CAN (ESP32 / Linux)
- **역할**: 플랫폼 독립적 CAN 통신 라이브러리
- **지원 플랫폼**: Linux (SocketCAN), ESP32 (TWAI)
- **주요 기능**:
  - 통합 CAN API (`can_api`)
  - 어댑터 패턴 기반 플랫폼 추상화
  - 콜백 기반 메시지 수신
  - 주기적 메시지 전송 (Job)
  - 메시지 인코딩/디코딩
- **상세 문서**:
  - [Library-CAN_ESP32/README.md](./Library-CAN_ESP32/README.md)
  - [Library-CAN_Linux/README.md](./Library-CAN_Linux/README.md)

---

## 통신 프로토콜

### Private CAN (TCU ↔ SCA ↔ DCU)
- **비트레이트**: 500 kbps
- **주요 메시지**:
  - 인증 요청/응답 (0x101~0x113)
  - 사용자 프로필 요청/전송 (0x201~0x209)
  - 시스템 리셋 (0x001~0x002)
  - 주행 상태 (0x005)
  - 졸음 이벤트 (0x003)

### Body CAN (DCU ↔ POW Devices)
- **비트레이트**: 500 kbps
- **주요 메시지**:
  - 시트/미러/휠 제어 명령
  - 상태 피드백 (20ms 주기)
  - 리셋 ACK

### REST API (App ↔ Server, TCU ↔ Server)
- **프로토콜**: HTTPS
- **인증**: Session 기반
- **주요 엔드포인트**:
  - 얼굴/NFC 등록 및 인증
  - 사용자 프로필 관리
  - 차량 예약 및 제어
  - AI 서비스 (측정/추천)

---

## 인증 플로우

### 1. 사전 등록 (앱)
1. 사용자가 앱에서 얼굴 이미지 촬영
2. 바디스캔으로 신체 치수 측정
3. NFC/BLE 디바이스 페어링
4. 서버에 프로필 저장

### 2. 현장 인증 (차량 측)
1. **착좌 감지**: POW-Seat → DCU
2. **인증 요청**: DCU → SCA (CAN 0x101)
3. **정보 요청**: SCA → TCU (CAN 0x102)
4. **세션 조회**: TCU → Server (REST API)
5. **NFC 인증**: SCA (I2C NFC 리더)
6. **BLE 인증**: SCA (BlueZ) ↔ 사용자 앱
7. **얼굴 인증**: SCA-AI (MediaPipe)
8. **인증 결과**: SCA → DCU (CAN 0x112)
9. **프로필 요청**: DCU → TCU (CAN 0x201)
10. **프로필 조회**: TCU → Server (REST API)
11. **프로필 적용**: TCU → DCU → POW Devices

### 3. 주행 중 모니터링
1. SCA-AI가 졸음 감지 (EAR/MAR/머리 기울기)
2. SCA → DCU 졸음 이벤트 전송 (CAN 0x003)
3. DCU-FE 경보 표시 및 사이렌 재생

---

## 상세 사용 시나리오

HYPERMOB 플랫폼의 전체 사용 흐름을 단계별로 상세하게 설명합니다.

### 시나리오 1: 사용자 등록

**목표**: 앱을 통해 사용자 프로필 생성 및 MFA 인증 정보 등록

#### 1단계: 초기 정보 입력
- 사용자가 HYPERMOB 앱 실행
- 회원가입 버튼 선택
- 이름, 이메일, 비밀번호 등 기본 정보 입력

#### 2단계: 얼굴 등록
- 얼굴 촬영 페이지로 자동 전환
- 전면 카메라로 얼굴 사진 촬영 (가이드라인 제공)
- 서버에 자동 업로드 → S3 저장
- Platform-AI가 468개 얼굴 랜드마크 자동 추출
- `user_id` 기반 얼굴 프로필 파일 생성 (`user1.txt`)

#### 3단계: 신체 측정 (바디스캔)
- 전신 촬영 페이지로 자동 전환
- 화면 가이드:
  - "정면을 보고 팔을 옆으로 벌려주세요"
  - 실루엣 가이드라인 표시
- 전면 카메라로 전신 사진 촬영
- 사용자 키 입력 (cm 단위)
- 서버에 자동 업로드 → S3 저장
- Platform-AI가 MediaPipe Pose로 신체 치수 자동 측정:
  - **상완 길이** (upper_arm): 어깨 ~ 팔꿈치
  - **전완 길이** (forearm): 팔꿈치 ~ 손목
  - **대퇴 길이** (thigh): 골반 ~ 무릎
  - **하퇴 길이** (calf): 무릎 ~ 발목
  - **몸통 길이** (torso): 어깨 ~ 골반

#### 4단계: AI 기반 차량 프로필 생성
- "맞춤형 차량 세팅을 준비중입니다..." 로딩 화면 표시
- Platform-AI가 Ridge 회귀 모델로 최적의 차량 설정 자동 계산:
  - **시트**: 위치(0~100%) / 각도(85~120°) / 전방높이(0~100%) / 후방높이(0~100%)
  - **사이드 미러**: 좌/우 Yaw(-30~30°) / Pitch(-20~20°)
  - **룸 미러**: Yaw(-15~15°) / Pitch(-10~10°)
  - **핸들**: 위치(0~100%) / 각도(30~60°)
- 서버 DB에 사용자 프로필 자동 저장

#### 5단계: MFA 디바이스 등록
- NFC 카드 등록 안내 화면
- BLE 디바이스 자동 페어링 (사용자 앱 자체가 BLE 디바이스로 동작)
- "등록이 완료되었습니다!" 메시지

#### 6단계: 등록 완료
- 메인 화면으로 자동 전환
- 사용자 프로필 카드 표시
- "대여 가능한 차량 보기" 버튼 활성화

**소요 시간**: 약 3~5분

---

### 시나리오 2: MFA 인증 및 차량 제어

**목표**: 차량 탑승 후 3단계 MFA 인증 및 자동 프로필 적용

#### Phase 1: 차량 접근 및 착좌 감지

**차량 접근**
- 사용자가 예약한 차량에 도착
- 차량 문 해제 (기존 스마트키 또는 앱 연동 가능)
- 운전석 착석

**착좌 감지 프로세스**
```
POW-Seat: 착좌 센서 압력 감지 (GPIO27, 디바운스 30ms)
POW-Seat → DCU: 착좌 상태 전송 (Body CAN, 20ms 주기)
DCU: 착좌 상태 확인
DCU → SCA: 인증 요청 (Private CAN 0x101, sig_flag=1)
```

**앱 MFA 화면 진입**
- 메인 화면에서 `대여 승인` 태그 차량 카드 선택
- "MFA 인증 시작" 버튼 표시
- 버튼 선택 시 인증 화면으로 전환
- 3개의 상태 카드 세로 배치:
  1. **NFC 인증** (회색, 대기 중)
  2. **BLE 인증** (회색, 대기 중)
  3. **얼굴 인증** (회색, 대기 중)

---

#### Phase 2: 3단계 MFA 인증 (총 20~30초)

**Step 1: NFC 인증 (5초 이내)**

통신 플로우:
```
SCA → TCU: 사용자 정보 요청 (CAN 0x102, sig_flag=1)
TCU → Server: GET /auth/session?car_id={car_id}&user_id={user_id}
Server: 세션 조회 및 NFC UID 응답
TCU → SCA: NFC UID 전송 (CAN 0x107, 8 bytes)
SCA: I2C NFC 리더 활성화
      - 폴링 주기: 150ms
      - 타임아웃: 6초
      - ISO14443A UID 우선 시도
      - 가능 시 ISO-DEP APDU 시도
```

사용자 동작:
- "NFC 카드 또는 스마트폰을 태그해주세요" 안내 메시지
- NFC 리더에 카드 또는 스마트폰 접촉
- UID 읽기 및 비교 검증

앱 화면 변화:
- NFC 상태 카드: 회색 → 파란색 (인증 중)
- 스피너 애니메이션 표시
- 인증 완료 시: 파란색 → 초록색, 체크마크 애니메이션

**Step 2: BLE 인증 (10초 이내)**

통신 플로우:
```
TCU → SCA: BLE Service ID 전송 (CAN 0x108, 6 bytes)
SCA: BlueZ로 BLE Peripheral 광고 시작
     - Service UUID: "12345678-0000-1000-8000-{12자리 해시}"
     - Characteristic: Write / Write-without-response
     - Advertising Interval: 100ms
앱: BLE 스캔 시작
    - RSSI 기반 근접 필터링 (-70dBm 이상)
    - Service UUID 매칭
앱 → SCA: Characteristic Write (챌린지 응답)
SCA: GDBus 콜백으로 응답 수신
     - 응답 데이터 검증
     - Service ID 비교
```

앱 화면 변화:
- BLE 상태 카드: 회색 → 파란색 (인증 중)
- "BLE 디바이스를 검색하는 중..." 메시지
- 인증 완료 시: 파란색 → 초록색, 체크마크 애니메이션

**Step 3: 얼굴 인증 (15초 이내)**

통신 플로우:
```
SCA → SCA-AI: 얼굴 인증 시작
              - input.txt에 "2" 기록
              - SCA-AI가 파일 폴링 (0.1초 주기)
```

Liveness 검사:
```
SCA-AI: 카메라 초기화 (1280x720, 30fps)
        MediaPipe FaceMesh 실행
        Liveness 검사 시작:
        - MAR(입 벌림) 임계값: 0.22
          - 랜드마크 13-14 / 78-308 거리 비율
        - 프레임 간 움직임 임계값: 0.003
          - 정규화된 랜드마크 평균 이동 거리
        - 최대 검사 프레임: 80
        - 타임아웃: 6초
```

사용자 동작:
- "카메라를 보며 입을 벌리거나 고개를 움직여주세요" 안내
- 카메라 화면에 얼굴 가이드라인 표시
- 사용자 동작 수행 (입 벌림 또는 고개 움직임)

Liveness 통과 시 프로필 매칭:
```
SCA-AI: user1.txt 파일 로드
        - 468개 랜드마크 인덱스 및 좌표 파싱
        - IPD(눈 간 거리) 기반 정규화
          - 랜드마크 33, 263 사용
          - 없으면 centroid 정규화
        카메라로부터 5프레임 수집
        - 각 프레임에서 랜드마크 추출
        - 5개 벡터 평균 계산 (노이즈 감소)
        z-score 정규화
        코사인 유사도 계산
        - 임계값: 0.99
        - 결과: output.txt에 "3"(성공) 또는 "4"(실패)
SCA: output.txt 폴링하여 결과 확인
```

앱 화면 변화:
- 얼굴 인증 상태 카드: 회색 → 파란색 (인증 중)
- 카메라 프리뷰 표시 (선택 사항)
- Liveness 단계 표시: "움직임 감지 중..."
- 매칭 단계 표시: "얼굴 인증 중..."
- 인증 완료 시: 파란색 → 초록색, 체크마크 애니메이션

**인증 완료 처리**

```
SCA → DCU: 인증 결과 전송 (CAN 0x112)
           - sig_flag: 0x01 (성공) / 0x00 (실패)
           - user_id: "USER123" (최대 7바이트)
           - 8바이트 초과 시 CAN 0x113으로 추가 전송
SCA → TCU: 인증 ACK (CAN 0x111)
           - idx: 인증 단계
           - state: 0(성공) / 1(실패)
TCU → Server: POST /auth/result
              - 인증 방법: "mfa"
              - 성공 여부
              - 타임스탬프
```

앱 화면:
- 모든 상태 카드가 초록색으로 전환
- "MFA 인증이 모두 완료되었습니다!" 대형 메시지
- 성공 애니메이션 (색종이 효과)
- 3초 자동 카운트다운
- 차량 제어 화면으로 자동 전환

**인증 실패 시 처리**
- 실패한 단계의 상태 카드: 빨간색, X 마크 표시
- 실패 사유 메시지:
  - NFC: "NFC 태그를 인식할 수 없습니다"
  - BLE: "BLE 연결에 실패했습니다"
  - 얼굴: "얼굴 인증에 실패했습니다" / "움직임이 감지되지 않았습니다" (Liveness 실패)
- "다시 시도" 버튼 활성화
- 3회 연속 실패 시:
  - 관리자에게 알림 전송
  - 5분 대기 후 재시도 가능

**소요 시간**: 20~30초 (정상 인증 시)

---

#### Phase 3: 사용자 프로필 자동 적용 (15~30초)

**프로필 조회**

통신 플로우:
```
DCU → TCU: 프로필 요청 (CAN 0x201, sig_flag=1)
TCU → Server: GET /users/{user_id}/profile
Server: DB 쿼리 및 프로필 데이터 조회
        - 시트: pos(%), angle(°), front_h(%), rear_h(%)
        - 미러: l_yaw, l_pitch, r_yaw, r_pitch, rm_yaw, rm_pitch
        - 핸들: pos(%), angle(°)
Server → TCU: JSON 응답
TCU: JSON 파싱 및 CAN 메시지 변환
TCU → DCU: 프로필 전송 (CAN 3개 메시지)
            - 0x202: 시트 프로필 (4 bytes)
            - 0x203: 미러 프로필 (6 bytes)
            - 0x204: 핸들 프로필 (2 bytes)
```

**DCU-FE 화면 전환**
- Auth View 페이드아웃
- Data Check View 페이드인 (1초 트랜지션)
- 상단 메시지: "사용자 프로필을 적용하는 중입니다..."
- 하단 프로그레스 바 (0 → 100%)
- 예상 소요 시간 표시: "약 20초 남음"

**POW 장치 제어 프로세스**

시트 제어 예시:
```
DCU → POW-Seat: 제어 명령 (Body CAN, BCAN_ID_DCU_SEAT_ORDER)
                - sig_seat_position: 50 (목표 위치 50%)
                - sig_seat_angle: 95 (목표 각도 95°)
                - sig_seat_front_height: 60 (전방 높이 60%)
                - sig_seat_rear_height: 55 (후방 높이 55%)

POW-Seat: 상태 머신 전환
          - Ready → Busy
          릴레이 제어 시작:
          - 현재 위치 읽기 (NVS)
          - 목표 위치와 비교
          - 방향 결정 (전진/후진)
          - PreHold 적용 (60ms)
          - 릴레이 ON/OFF 반복
          - 엔코더 값 모니터링

POW-Seat → DCU: 상태 피드백 (Body CAN, 20ms 주기)
                - BCAN_ID_POW_SEAT_STATE
                - sig_seat_position: 현재 위치 (0~100%)
                - sig_seat_angle: 현재 각도 (85~120°)
                - sig_seat_front_height: 현재 전방 높이
                - sig_seat_rear_height: 현재 후방 높이
                - sig_status: 0(NotReady) / 1(Ready) / 2(Busy)

POW-Seat: 목표 도달 감지
          - 허용 오차: ±1% (위치), ±1° (각도)
          - 릴레이 OFF
          - NVS에 현재 위치 저장 (2초 간격)
          - Busy → Ready 전환
```

미러 제어 (유사 프로세스):
- 사이드 미러: 릴레이 제어 (6개 방향)
- 룸 미러: PCA9685 서보 제어 (PWM)

핸들 제어 (유사 프로세스):
- 릴레이 제어 (전/후, 상/하)

**DCU-FE 실시간 상태 표시**

화면 구성:
- 3개의 진행 카드 (시트/미러/핸들)
- 각 카드 내용:
  - 디바이스 아이콘
  - 현재 값 / 목표 값 표시
  - 프로그레스 바 (수평)
  - 퍼센티지 텍스트

실시간 업데이트:
```
DCU: CAN 메시지 수신 (20ms 주기)
     - 각 POW 장치의 현재 상태
     IPC 전송 (JSON)
     - topic: "datacheck/progress"
     - data: {
         seat: {pos: 45, angle: 92, ...},
         mirror: {l_yaw: 10, ...},
         wheel: {pos: 60, angle: 40}
       }
DCU-FE: IPC 수신
        - 슬라이더 위치 업데이트
        - 프로그레스 바 계산
        - 애니메이션 적용 (부드러운 전환)
```

예시 화면:
```
┌─────────────────────────────────────┐
│ 사용자 프로필을 적용하는 중입니다...    │
└─────────────────────────────────────┘

┌─────────────────────────────────────┐
│ 🪑 시트                              │
│ 위치: 45% → 50%  [=======>  ] 90%  │
│ 각도: 92° → 95°  [=======>  ] 90%  │
└─────────────────────────────────────┘

┌─────────────────────────────────────┐
│ 🪞 미러                              │
│ 좌측: 10° → 15° [=========> ] 85%  │
│ 우측: -5° → 0°  [========>  ] 80%  │
└─────────────────────────────────────┘

┌─────────────────────────────────────┐
│ 🎮 핸들                              │
│ 위치: 60% → 55%  [==========>] 100%│
└─────────────────────────────────────┘

[====================      ] 75%
약 5초 남음
```

**적용 완료**

```
DCU: 모든 POW 장치 Ready 상태 확인
     - sig_status == 1 (Ready)
     IPC 전송
     - topic: "datacheck/complete"
     - data: {success: true}

DCU-FE: "프로필 적용이 완료되었습니다!" 메시지
        - 성공 효과음 재생
        - 2초 대기
        - Data Check View → Main View 자동 전환
```

**소요 시간**: 15~30초 (장치 이동 거리에 따라 변동)

---

#### Phase 4: 수동 조정 및 운전 시작 (1~3분)

**Main View 화면 구성**

레이아웃:
```
┌─────────────────────────────────────────────┐
│               [Main View]                    │
├─────────────────┬───────────────────────────┤
│  상태 대시보드   │      제어 패널              │
│                 │                            │
│  🪑 시트        │   [▲] [▼] [◀] [▶]        │
│   위치: 50%     │   시트 전후 / 상하 조절      │
│   각도: 95°     │                            │
│                 │   [─] [+]                  │
│  🪞 미러        │   등받이 각도 조절           │
│   좌측: 15°     │                            │
│   우측: 0°      │   [ 3x3 그리드 버튼 ]      │
│                 │   미러 방향 조절             │
│  🎮 핸들        │                            │
│   위치: 55%     │   [◀] [▶] [▲] [▼]        │
│   각도: 40°     │   핸들 전후 / 각도 조절      │
│                 │                            │
│                 │   [ 프로필 저장 ]           │
└─────────────────┴───────────────────────────┘
│           [ 운전 시작 ]                      │
└─────────────────────────────────────────────┘
```

**시트 미세 조정**

사용자 동작:
- 시트 전방 버튼 [▶] 클릭 (터치 다운)

통신 플로우:
```
사용자: 버튼 터치 다운
DCU-FE: IPC 전송
        - topic: "main/seat/jog"
        - data: {axis: "position", dir: 1}
DCU: CAN 전송 (Body CAN)
     - BCAN_ID_DCU_SEAT_BUTTON
     - sig_jog_position: 1 (전방 이동)
POW-Seat: Jog 모드 진입
          - 60ms PreHold 적용
          - 릴레이 ON (전방)
          - 엔코더 모니터링
POW-Seat → DCU: 상태 피드백 (20ms 주기)
                - sig_seat_position: 50 → 51 → 52...
DCU → DCU-FE: IPC 전송
              - topic: "main/seat/state"
              - data: {pos: 51, angle: 95, ...}
DCU-FE: 슬라이더 업데이트 (부드러운 애니메이션)

사용자: 버튼 터치 업
DCU-FE: IPC 전송
        - topic: "main/seat/jog"
        - data: {axis: "position", dir: 0}
DCU: CAN 전송
     - sig_jog_position: 0 (정지)
POW-Seat: Jog 해제
          - 릴레이 즉시 OFF
          - 현재 위치 NVS 저장 (2초 후)
          - 최종 위치: 55%
```

조정 가능 항목:
- **시트 전후**: 1% 단위 조정 (0~100%)
- **등받이 각도**: 1° 단위 조정 (85~120°)
- **전방/후방 높이**: 1% 단위 조정 (0~100%)

**미러 조정**

3x3 그리드 버튼:
```
┌───┬───┬───┐
│ ↖ │ ↑ │ ↗ │  좌측 사이드 미러
├───┼───┼───┤
│ ← │ ● │ → │  ● = 중앙 (현재 위치)
├───┼───┼───┤
│ ↙ │ ↓ │ ↘ │
└───┴───┴───┘
```

동작:
- 각 방향 버튼 클릭 시 Yaw/Pitch 조합 조정
- 예: ↗ 클릭 시 Yaw +5°, Pitch +3°
- 룸 미러는 서보 모터로 부드럽게 조정

**핸들 조정**

- 전/후 위치: 체격에 맞춤 (1% 단위)
- 각도 조정: 운전 자세에 맞춤 (1° 단위)
- 우측 앵글 카메라로 촬영 시 변화 확인 가능

**수동 조정 자동 저장**

```
POW-Seat: 수동 조정 완료 감지
          - 2초간 입력 없음
          수동 조정 보고 (Body CAN)
          - BCAN_ID_DCU_USER_PROFILE_SEAT_UPDATE (0x206)
          - sig_seat_position: 55
          - sig_seat_angle: 98
          - sig_seat_front_height: 62
          - sig_seat_rear_height: 56
DCU → POW-Seat: ACK 전송 (CAN 0x209)
DCU → TCU: 수동 조정 데이터 중계
TCU → Server: POST /vehicles/{car_id}/settings/manual
              - user_id: "USER123"
              - seat: {pos: 55, angle: 98, ...}
              - mirror: {...}
              - wheel: {...}
              - timestamp
Server: DB 업데이트
        - 사용자 프로필에 수동 조정값 반영
        - 다음 이용 시 자동 적용
```

**운전 시작**

사용자 동작:
- "운전 시작" 대형 버튼 선택

시스템 전환:
```
DCU-FE: IPC 전송
        - topic: "main/drive/start"
DCU: 주행 상태 전송 (Private CAN)
     - BCAN_ID_DCU_SCA_DRIVE_STATUS (0x005)
     - sig_flag: 1 (주행 중)
SCA: 상태 머신 전환
     - Idle → Drive
     - 인증 모드 종료
     - 주행 모드 진입
SCA-AI: 졸음 감지 시작
        - drowsiness.py 실행
        - 카메라 초기화
        - EAR/MAR/머리 기울기 모니터링 시작
DCU-FE: Main View 화면 전환
        - 제어 버튼 비활성화 (안전 잠금)
        - "운전 중" 배지 표시
        - 졸음 감지 상태 카드 표시
        - 주행 시간 타이머 시작
```

운전 중 화면:
```
┌─────────────────────────────────────────────┐
│            [Main View - 운전 중]             │
├─────────────────────────────────────────────┤
│  🚗 주행 중                                  │
│  운전 시간: 00:05:23                         │
│                                              │
│  😴 졸음 감지: 정상                          │
│  ├─ 눈 개폐 (EAR): 0.28 ✓                   │
│  ├─ 하품 (MAR): 0.15 ✓                      │
│  └─ 머리 기울기: 5° ✓                       │
│                                              │
│  [ 제어 패널 비활성화 ]                       │
│  (주행 중에는 조정할 수 없습니다)               │
└─────────────────────────────────────────────┘
│           [ 운전 종료 ]                      │
└─────────────────────────────────────────────┘
```

**소요 시간**: 1~3분 (사용자 선택)

---

#### Phase 5: 주행 중 졸음 감지 (실시간)

**SCA-AI 실시간 모니터링**

```python
# drowsiness.py 주요 로직
while True:
    ret, frame = cap.read()
    results = face_mesh.process(frame)

    if results.multi_face_landmarks:
        landmarks = results.multi_face_landmarks[0]

        # EAR 계산 (Eye Aspect Ratio)
        left_eye = get_eye_landmarks(landmarks, LEFT_EYE_INDICES)
        right_eye = get_eye_landmarks(landmarks, RIGHT_EYE_INDICES)
        ear = (calculate_ear(left_eye) + calculate_ear(right_eye)) / 2

        # MAR 계산 (Mouth Aspect Ratio)
        mouth = get_mouth_landmarks(landmarks, MOUTH_INDICES)
        mar = calculate_mar(mouth)

        # 머리 기울기 계산
        roll, pitch = calculate_head_pose(landmarks)

        # 졸음 판정
        if ear < EAR_THRESH and ear_counter > EAR_CONSEC_FRAMES:
            drowsy = True
        if mar > MAR_THRESH:
            drowsy = True
        if abs(roll) > HEAD_TILT_THRESH or abs(pitch) > HEAD_TILT_THRESH:
            drowsy = True

        # 파일에 기록
        with open('drowsiness.txt', 'w') as f:
            f.write('1' if drowsy else '0')
```

졸음 지표:
- **EAR** (Eye Aspect Ratio): 눈 개폐 상태
  - 정상: 0.25 이상
  - 졸음: 0.25 미만이 연속 3초 (90프레임 @ 30fps)
  - 계산: (수직 거리) / (수평 거리)
- **MAR** (Mouth Aspect Ratio): 하품 감지
  - 정상: 0.2 미만
  - 하품: 0.2 이상이 1초 이상
  - 계산: (입 높이) / (입 너비)
- **머리 기울기**: Roll/Pitch 각도
  - 정상: ±15도 이내
  - 졸음: ±15도 초과가 2초 이상

**졸음 판정 알고리즘**

```
if (EAR < 0.25 for 3초) OR
   (MAR > 0.2 for 1초) OR
   (|Roll| > 15° for 2초) OR
   (|Pitch| > 15° for 2초):
    drowsiness = True
```

**SCA-Core 상태 확인**

```cpp
// SCA-Core drowsiness 파일 폴링
std::ifstream drowsy_file("../SCA-AI/drowsiness.txt");
int status;
drowsy_file >> status;

if (status == 1 && !drowsy_alert_sent) {
    // 졸음 이벤트 전송
    CanMessage msg = {0};
    msg.sca_dcu_driver_event.sig_flag = 1; // 졸음 감지

    CanFrame frame = can_encode_pcan(
        PCAN_ID_SCA_DCU_DRIVER_EVENT, // 0x003
        &msg,
        1
    );

    can_send("can0", frame, 1000);
    drowsy_alert_sent = true;
}
```

**DCU-FE 경보 표시**

```cpp
// DCU-FE 졸음 이벤트 수신 처리
void MainView::onDrowsyEvent() {
    // WarningDialog 표시
    WarningDialog* dialog = new WarningDialog(this);
    dialog->setTitle("졸음 운전 감지");
    dialog->setMessage("졸음 운전이 감지되었습니다!\n휴게소에서 휴식을 취하세요.");
    dialog->setSeverity(WarningDialog::HIGH);

    // 사이렌 재생
    QMediaPlayer* player = new QMediaPlayer(this);
    player->setMedia(QUrl("qrc:/sounds/siren.mp3"));
    player->setVolume(100);
    player->play();

    // 모달 표시 (자동 닫힘 없음)
    dialog->exec();
}
```

경보 화면:
```
┌─────────────────────────────────────────────┐
│            ⚠️ 졸음 운전 감지                 │
├─────────────────────────────────────────────┤
│                                              │
│  졸음 운전이 감지되었습니다!                   │
│  휴게소에서 휴식을 취하세요.                   │
│                                              │
│  감지 시각: 14:35:22                         │
│  주행 시간: 01:25:13                         │
│                                              │
│  [ 확인 ]                                    │
│                                              │
└─────────────────────────────────────────────┘
```

특징:
- 전체 화면 모달 (운전에 집중 필요)
- 사이렌 음향 재생 (siren.mp3)
- 수동 확인 필요 (자동 닫힘 없음)
- 졸음 상태 지속 시 반복 경보 (2분 간격)

**로그 기록**

```
DCU → TCU: 졸음 이벤트 로그
TCU → Server: POST /vehicles/{car_id}/events/drowsiness
              - user_id: "USER123"
              - timestamp: "2025-10-29T14:35:22Z"
              - ear: 0.18
              - mar: 0.25
              - head_tilt: 18°
Server: DB 저장
        - 운전자 안전 점수 업데이트
        - 누적 졸음 횟수 기록
        - 휴식 권장 알림 생성
```

**사용자 반응**

1. 경보 확인 버튼 클릭
2. 경보창 닫힘
3. 졸음 상태 지속 시 2분 후 재경보
4. 휴게소 진입 및 휴식 권장

**소요 시간**: 3~5초 내 감지 및 경보

---

#### Phase 6: 운전 종료

**주행 종료**

사용자 동작:
- 차량 정지 및 시동 OFF
- DCU-FE "운전 종료" 버튼 선택

시스템 처리:
```
DCU-FE: IPC 전송
        - topic: "main/drive/stop"
DCU: 주행 상태 종료 (Private CAN)
     - BCAN_ID_DCU_SCA_DRIVE_STATUS (0x005)
     - sig_flag: 0 (정지)
SCA: 상태 머신 전환
     - Drive → Idle
SCA-AI: 졸음 감지 중지
        - drowsiness.py 종료
        - 카메라 릴리즈
DCU-FE: Main View → 초기 화면 전환
```

**소요 시간**: 즉시

---

## 개발 환경 설정

### Prerequisites

#### 공통
- Git 2.0+
- CMake 3.16+
- GCC/G++ (C++17 지원)

#### Platform
- Android Studio Hedgehog (2023.1.1)+
- Node.js 18+
- Python 3.9+
- AWS CLI 2.0+
- Docker Desktop

#### Embedded (Linux)
- Qt 5.15+
- libcurl, cJSON, libsocketcan
- BlueZ, libnfc
- Miniconda (SCA-AI)

#### Embedded (ESP32)
- Arduino IDE 또는 PlatformIO
- ESP-IDF

### 저장소 클론

```bash
git clone https://github.com/HYPER-MOB/2025ESWContest_mobility_6099.git
cd 2025ESWContest_mobility_6099
```

### Subtree 관리

이 프로젝트는 git subtree를 사용하여 각 서브 시스템을 통합 관리합니다.

#### Subtree 업데이트 (Pull)
```bash
# 모든 subtree 업데이트
./setup_subtree.sh

# 또는 개별 subtree 업데이트는 .subtree/ 폴더의 config 파일에서
# MODE를 'pull'로 설정 후 실행
```

#### Subtree 푸시 (Push)
```bash
# .subtree/ 폴더의 해당 config 파일에서 MODE를 'push'로 설정 후 실행
./setup_subtree.sh
```

각 subtree의 독립적인 개발은 원본 저장소에서 진행하며, 이 통합 저장소는 릴리스 및 통합 테스트 용도로 사용됩니다.

---

## 빌드 및 실행

각 컴포넌트별 상세한 빌드 및 실행 방법은 해당 디렉토리의 README.md를 참고하세요.

### Quick Start

#### 1. Platform-Server 배포
```powershell
cd Platform-Server
.\scripts\Deploy-Complete-Platform.ps1
```

#### 2. Android 앱 빌드
```bash
cd Platform-Application/android
./gradlew assembleDebug
```

#### 3. TCU 빌드 및 실행
```bash
cd TCU-Core
mkdir build && cd build
cmake ..
make
sudo ./hypermob-tcu can0 https://your-api.com/v1 CAR001
```

#### 4. DCU 빌드 및 실행
```bash
# Backend
cd DCU-Core
mkdir build && cd build
cmake ..
make
sudo ./ipc_demo

# Frontend
cd DCU-FE
mkdir build && cd build
cmake ..
make
./dcu_ui_frontend
```

#### 5. SCA 빌드 및 실행
```bash
# Core
cd SCA-Core
mkdir build && cd build
cmake ..
make
sudo setcap cap_net_admin+ep ./rpi_can_router
./rpi_can_router

# AI
cd SCA-AI
python3 faceauth.py  # 얼굴 인증
python3 drowsiness.py  # 졸음 감지
```

#### 6. POW 장치 플래싱
```bash
# Arduino IDE에서 각 .ino 파일 열기
# 보드: ESP32 Dev Module
# 업로드
```

---

## 기술 스택 요약

### Platform
- **Frontend**: Kotlin, Jetpack Compose, ML Kit
- **Backend**: Node.js, Python, AWS Lambda, API Gateway, RDS
- **AI**: MediaPipe, scikit-learn

### Embedded
- **TCU**: C, REST API (libcurl, cJSON)
- **DCU**: C++17, Qt 5.15
- **SCA**: C++, Python, BlueZ, libnfc, MediaPipe
- **POW**: Arduino (ESP32), TWAI

### Communication
- **CAN**: SocketCAN (Linux), TWAI (ESP32)
- **Network**: REST API (HTTPS), IPC (QLocalSocket)
- **Wireless**: BLE (BlueZ), NFC (ISO14443A)

---

## 라이선스

Copyright (c) 2025 HYPERMOB Team. All rights reserved.

이 프로젝트는 2025 임베디드 소프트웨어 경진대회 출품작으로, 교육 및 연구 목적으로만 사용할 수 있습니다.

---

## 문의

프로젝트 관련 문의사항은 각 서브 시스템의 담당자에게 연락하거나, GitHub Issues를 통해 문의해주세요.

**Last Updated**: 2025-10-29
**Version**: 1.0.0
