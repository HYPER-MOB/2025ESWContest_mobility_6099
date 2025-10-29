# MyDrive3DX Platform Application

> **개인화 주행 플랫폼**을 위한 통합 애플리케이션 저장소

MyDrive3DX는 3D 바디스캔 기술과 다중 인증(MFA)을 결합하여 개인 맞춤형 주행 경험을 제공하는 차세대 차량 공유 플랫폼입니다.

## 프로젝트 구성

```
Platform-Application/
├── android/                # Android 앱 (Kotlin + Jetpack Compose)
└── .legacy/                # 레거시 코드 및 아카이브
    ├── web/                # React 웹 프로토타입 (UI/UX 검증 완료)
    └── android_mvp/        # MFA 인증 테스트용 MVP
```

## Android Application (`/android`)

Kotlin + Jetpack Compose로 구축된 네이티브 Android 앱입니다.

**핵심 기능**
- Multi-Factor Authentication (얼굴 인식 + NFC + BLE)
- 3D 바디스캔 (ML Kit 기반 신체 측정)
- 스마트 차량 대여 및 원격 제어
- 졸음 감지 시스템
- 8축 자동 차량 세팅

**기술 스택**
- Kotlin 1.9 + Jetpack Compose
- Clean Architecture + MVVM
- Hilt (DI) + Coroutines + Flow
- ML Kit (Face/Pose Detection)
- CameraX + Retrofit2 + Room

**시작하기**
```bash
cd android
./gradlew installDebug
```

📖 자세한 내용: [android/README.md](./android/README.md)

---

## 시스템 아키텍처

```
[모바일 앱]  ←→  [API 서버]  ←→  [TCU (RPi)]  ←→  [DCU]  ←→  [Body CAN]
                                     ↕
                                 [SCA (Jetson)]
```

### MFA 인증 플로우

1. **사전 등록** (앱)
   - 얼굴 이미지 업로드
   - 체형 데이터 입력 (7가지 신체 치수)
   - NFC/BT 디바이스 페어링

2. **현장 인증** (차량 측)
   - Power Seat 착좌 감지 → DCU
   - DCU → SCA 얼굴 인식 요청
   - SCA ↔ 서버 사용자 정보 조회
   - 스마트키 + 얼굴 + NFC/BT 검증
   - 인증 완료 → 프로필 자동 적용

3. **앱 역할**
   - 사전 등록: 서버에 생체 정보 업로드
   - 현장 인증: SCA의 NFC/BT 챌린지에 응답 (백그라운드)

---

## 개발 환경

- Android Studio Hedgehog (2023.1.1) 이상
- JDK 17
- Android SDK 34
- Gradle 8.2

---

## 주요 기능

### 차량 대여
- 위치·차종·옵션 기반 검색 및 필터링
- 실시간 예약 및 배차 관리
- 예약 상태 타임라인 (requested → approved → picked)

### 사전 등록
- 3단계 진행 UI (Progress bar + 체크마크)
- 셀카 촬영 (CameraX)
- 체형 입력 (키, 앉은키, 어깨폭, 팔길이, 시야각 등)
- NFC/BT 디바이스 페어링

### 차량 제어
- MFA 게이트 (인증 전 잠금 화면)
- 안전 게이트 (주행 중 제어 차단)
- 시트 조정 (전후, 상하, 등받이)
- 미러 8방향 조정 (3x3 그리드)
- 프로필 자동 적용 버튼
- 실시간 제어 로그

### MFA 인증
- 얼굴 인식 (ML Kit Face Detection)
- NFC 태그 인증 (ISO14443A)
- BLE 디바이스 페어링 및 챌린지-응답

---

## 프로젝트 상태

| 모듈 | 상태 | 설명 |
|------|------|------|
| Android App | 🚧 개발 중 | 핵심 기능 구현 완료, 통합 테스트 진행 중 |
| Backend API | 📝 계획 | API 명세 작성 완료, 구현 대기 |

---

## 빌드 및 배포

```bash
cd android

# Debug 빌드 (Fake 데이터 사용)
./gradlew assembleDebug

# Release 빌드 (실제 API 연결)
./gradlew assembleRelease
```

---

## 기여 가이드

### 커밋 메시지 규칙
```
feat: 새로운 기능 추가
fix: 버그 수정
docs: 문서 수정
refactor: 코드 리팩토링
test: 테스트 추가
style: 코드 포맷팅
chore: 빌드 설정 변경
```

### 코드 컨벤션
- **Kotlin**: [Kotlin Coding Conventions](https://kotlinlang.org/docs/coding-conventions.html)
- **클래스명**: PascalCase
- **함수/변수명**: camelCase
- **상수**: UPPER_SNAKE_CASE

---

## 참고 문서

- [Android README](./android/README.md) - Android 앱 개발 가이드
- [Architecture Guide](./android/ARCHITECTURE.md) - 아키텍처 상세 설명
- [Features Documentation](./android/FEATURES.md) - 기능별 상세 가이드
- [Setup Guide](./android/SETUP_GUIDE.md) - 개발 환경 설정

### Legacy 참고
- [Web Prototype](./.legacy/web/README.md) - React 웹 프로토타입 (UI/UX 검증 완료)
- [Android MVP](./.legacy/android_mvp/README.md) - MFA 인증 테스트용 MVP

---

## 라이선스

Copyright © 2025 HYPERMOB. All rights reserved.

---

## 문의

프로젝트 관련 문의사항은 개발팀에게 연락하세요.

**Last Updated**: 2025-10-29
**Version**: 1.0.0
