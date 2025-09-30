# MyDrive3DX Mobile – PRD

> **웹 프로토타입 검증 완료** (2025-01): 핵심 UI/UX 구현 및 검증. 본 문서는 React Native 모바일 앱 개발을 위한 PRD입니다.

## 1. 제품 개요

**제품명**: MyDrive3DX Mobile

**목표**: 차량 대여, 사용자 등록(MFA 사전 준비), 차량 제어를 하나의 모바일 앱에서 제공

**핵심 가치**
- **초보안**: 차량 측 3단계 MFA (스마트키 → 얼굴 → NFC/BT). 앱은 사전 등록 데이터 제공
- **초개인화**: 체형 기반 프로필 자동 반영 (앉은키·어깨폭·팔 길이·시야각)
- **연결성**: DCU↔SCA↔TCU, Private/Body CAN 분리 + 클라우드 동기화

---

## 2. 사용자 & 시나리오

**대상**: 카셰어/렌터카, 사내 차량, 발렛/모빌리티 서비스, 고령자/체형 편차 사용자

**주요 플로우**
1. 대여 신청 → 관리자 배차 승인 → 현장 도착
2. **사전 등록** (앱): 셀카·체형 입력 → 서버 업로드 → TCU가 SCA에 전달
3. 차량 탑승 → 착좌 감지 → **MFA (차량 자동 수행)**
   - 스마트키 확인 (차량)
   - 얼굴 매칭 (SCA ↔ TCU ↔ 서버)
   - NFC/BT 확인 (SCA ↔ 모바일)
4. 인증 성공 → **프로필 자동 적용** (DCU ↔ Body CAN)
5. 앱에서 시트/미러 미세 조정 (정차·안전 조건 하)
6. 운행 종료/시트 이탈 → 세션 초기화

---

## 3. 범위

### In-scope (v1.0)

**✅ 웹 프로토타입 구현 완료**:
- ✅ 로그인 (dummy: id=dev, pw=dev, localStorage 인증)
- ✅ 홈 화면 (예약 현황, MFA 상태, 빠른 액션)
- ✅ 차량 대여 (목록·검색·상세·예약 타임라인)
- ✅ 사전 등록 3단계 (셀카 → 체형 입력 → NFC/BT 페어링)
  - 진행률 표시 (Progress bar)
  - 단계별 완료 체크
- ✅ 차량 제어 (MFA/안전 상태 체크)
  - 시트 조정: 슬라이드(-10~10), 높이(0~5), 등받이(90~120°)
  - 미러 8방향 조정 (좌/우 독립)
  - 프로필 자동 적용 버튼
  - 실시간 로그 표시
- ✅ 설정 화면 (메뉴 목록)
- ✅ StatusBadge 컴포넌트
- ✅ Toast 알림 시스템
- ✅ 다크 테마 디자인 시스템

**모바일 앱에 추가 필요**:
- 실제 카메라 API 연동 (react-native-vision-camera)
- NFC/BT 하드웨어 연동 (react-native-nfc-manager, ble-plx)
- WebSocket 실시간 MFA 상태 동기화
- 서버 API 연동 (대여, 등록, 제어)

### Out-of-scope (v1.0)
- 로그인/회원가입 구축
- 결제/과금 (외부 링크만)
- 실시간 졸음 감지 (차량 측)
- 자동 3D 스캔 (SCA 수행, 앱은 뷰/수정)

---

## 4. KPI

| 지표 | 목표 |
|------|------|
| 가입→등록 완료 전환율 | ≥70% |
| 첫 배차 소요 시간 | <3분 |
| 제어 명령 성공률 | ≥99% (타임아웃 제외) |
| MFA 전체 소요 (차량 측) | ≤3–5초 |
| 크래시 프리 세션 | ≥99.5% |

---

## 5. 주요 사용자 스토리

| ID | 스토리 | 수용 기준 |
|----|--------|-----------|
| US-LOGIN-01 | 이메일/비번 로그인 | 비밀번호 정책·토큰 만료·재로그인 UX |
| US-RENT-01 | 차량 예약/취소 | 재고 검증, 상태 머신 (요청→승인→수령→반납→취소) |
| US-REG-01 | 셀카·체형·NFC/BT 정보 등록 → 서버 전달 | 화질 체크, NFC 기기 가이드, 필수 항목 검증 |
| US-MFA-01 | 차량 착좌 시 MFA 자동 수행, 앱은 NFC/BT 챌린지 응답 | SCA 요청 시 앱이 서명/토큰 제공 (백그라운드) |

---

## 6. 기능 요구사항

### 6.1 인증
- 이메일·비밀번호 (정책: 최소 8자·문자 조합), 재설정
- Secure Storage에 리프레시 토큰 저장
- 역할: 일반 사용자, 운영자 (관리자 배차용)

### 6.2 대여
- 목록 (위치·차종·옵션·잔여), 기간 선택, 예약/취소
- 배차 상태: 관리자 승인 대기 → 승인 → 수령 → 반납

### 6.3 사전 등록 (MFA 준비)
**목적**: 차량 MFA를 위한 데이터 서버 등록

- **셀카**: 얼굴 가이드·품질 검증 → 암호화 업로드 → 서버가 TCU 경유 SCA 전달
- **체형**: 키·앉은키·어깨폭·팔길이·시야·미러각 (수동 입력, 추후 카메라 보조)
- **NFC/BT 페어링**
  - NFC: 디바이스 ID, 공개키 등록 (NDEF는 차량 측이 생성)
  - BT: MAC 주소, 페어링 토큰 등록
  - 앱은 백그라운드에서 SCA 챌린지에 응답
- **프로필**: 서버·로컬 동시 보관 (최근 N개 캐시), 차종 보정 테이블

### 6.4 MFA (차량 자동 수행, 앱 역할)
**전체 플로우** (flowchart.md 기반)
1. 사용자 착좌 → Power Seat 감지 → DCU에 신호
2. DCU → SCA: 얼굴 인식 요청 (`DCU_USER_FACE_REQ`)
3. SCA → TCU: 사용자 인증 정보 요청 (`SCA_USER_INFO_REQ`)
   - TCU → 서버: 차량 ID 전달
   - 서버 → TCU: 사용자 ID, 얼굴 데이터, 프로필 전달
4. SCA: **3단계 MFA 수행**
   - ① 스마트키 확인 (차량 내장)
   - ② 얼굴 매칭 (SCA 카메라)
   - ③ NFC/BT 확인 (SCA ↔ 모바일)
5. SCA → DCU: 인증 결과 + 사용자 ID (`SCA_AUTH_RESULT`)
6. DCU → Body CAN: 프로필 적용 명령

**앱 역할**
- **사전**: 얼굴·체형·NFC/BT 디바이스 정보 서버 등록
- **현장**: SCA의 NFC/BT 챌린지 수신 시 서명/토큰 응답 (백그라운드)
- **보안**: 응답은 디바이스 키로 서명, 일회성 토큰 사용

### 6.5 제어
**전제 조건**: 정차·이그니션 안전·도어 해제, 프로필 적용 시 DCU↔TCU↔B-CAN, 초기화 조건 시 세션 종료

**명령**
- Seat: forward/backward/height/backrestAngle, preset
- Mirror: left/right tilt, room angle
- Steering: tilt/telescopic (옵션)

**피드백**: accepted→in-progress→completed/failed, 타임아웃 재시도

---

## 7. 시스템 아키텍처

### 구조
```
[모바일(RN)] ↔ [API 서버] ↔ [TCU(RPi)] ↔ [DCU] ↔ [Body CAN]
                              ↕
                            [SCA(Jetson)]
```

**MFA 플로우** (flowchart 기반)
1. Power Seat → DCU (착좌 감지)
2. DCU → SCA (얼굴 인식 요청)
3. SCA ↔ TCU ↔ 서버 (사용자 정보 조회)
4. SCA: 스마트키 + 얼굴 + NFC/BT 검증
5. SCA → DCU (인증 결과)
6. DCU → Body CAN (프로필 적용)

### 통신
- 모바일–서버: HTTPS/HTTP2, JWT, WebSocket
- 서버–TCU: MQTT/HTTPS
- TCU–DCU–SCA: CAN (Private CAN)
- DCU–MCU: CAN (Body CAN, 별도 spec)
- SCA–모바일: NFC (NDEF), BLE

### CAN 메시지 (flowchart 기반)

| 번호 | 메시지 | 설명 |
|------|--------|------|
| 3 | `DCU_USER_FACE_REQ` | DCU → SCA: 얼굴 인식 요청 |
| 4 | `SCA_USER_INFO_REQ` | SCA → TCU: 사용자 정보 조회 요청 |
| 5 | `SCA_AUTH_STAGE` | SCA → DCU: 인증 단계 정보 (주기 전송) |
| 6 | ACK | TCU → SCA: 사용자 정보 전달 완료 |
| 7 | `SCA_AUTH_RESULT` | SCA → DCU: 인증 결과 + 사용자 ID |

### 제어 명령 스펙

**명령**
```json
{
  "commandId": "uuid", "vehicleId": "V123", "type": "SEAT_SET",
  "params": { "backrestAngle": 102, "height": 3, "slide": -1 },
  "profileId": "P456", "issuedBy": "user|system", "ts": 1736423000
}
```

**이벤트**
```json
{
  "commandId": "uuid", "status": "completed|failed|timeout",
  "detail": "applied", "ts": 1736423004
}
```

---

## 8. 데이터 모델

| 엔티티 | 주요 필드 |
|--------|-----------|
| User | id, email, pw_hash, device_id, device_pub_key, consent |
| Profile | id, userId, bodyMetrics{height,sitHeight,shoulder,arm,view}, seatPreset, mirrorPreset |
| Vehicle | id, model, location, capabilities{seat,mirror,steering}, smartkey_id |
| Rental | id, userId, vehicleId, period{start,end}, status{requested,approved,picked,returned,canceled} |
| MFALog | id, vehicleId, userId, stage{smartkey,face,nfc_bt}, result, ts |
| ControlLog | id, vehicleId, profileId, command, status, ts |

---

## 9. 보안

| 항목 | 내용 |
|------|------|
| MFA | 차량 측 3단계 (스마트키 + 얼굴 + NFC/BT) |
| 암호화 | TLS 1.3 전 구간, at-rest 암호화 |
| 키 관리 | Secure Enclave/Keystore, react-native-keychain |
| 생체 데이터 | 해시/벡터만 저장, 원본 즉시 폐기 |
| NFC/BT | 디바이스 키 서명, 일회성 토큰, 재사용 방지 |
| 권한 | NFC·카메라·BT 최소화, 목적 고지·동의 |

---

## 10. 비기능 요구사항

| 항목 | 목표 |
|------|------|
| 응답 시간 | 주요 화면 <200ms, 명령 타임아웃 2–3s (재시도 최대 2회) |
| 오프라인 | 최근 프로필·예약 N개 캐시, 재연결 시 동기화 |
| 접근성 | VoiceOver/TalkBack, 큰 버튼, 상태 읽기 |
| 현지화 | ko-KR(기본), en-US 준비 |
| 분석 | 화면/액션 이벤트, 전환 퍼널 (가입→등록→배차) |

---

## 11. 리스크 & 완화

| 리스크 | 완화 방안 |
|--------|-----------|
| NFC 미지원 단말 | BLE fallback |
| 네트워크 음영 | 프로필 캐시·로컬 적용, 사후 동기화 |
| 차종별 제어 오차 | 보정 테이블/캘리브레이션 DB |
| MFA 실패 (얼굴 미인식) | 재시도 가이드, 관리자 수동 승인 |

---

## 12. 기술 스택

### 웹 프로토타입 (검증 완료)
| 레이어 | 기술 | 비고 |
|--------|------|------|
| 코어 | React + TypeScript + Vite | - |
| 라우팅 | React Router v6 | - |
| 상태 | React Hooks (useState) | 단순 로컬 상태 |
| UI | shadcn/ui + Tailwind CSS | 다크 테마, 커스텀 토큰 |
| 알림 | Toaster + Sonner | - |
| 아이콘 | lucide-react | - |
| 빌드 | Vite | - |

### React Native 앱 (목표)
| 레이어 | 기술 | 버전/비고 |
|--------|------|-----------|
| 코어 | React Native + TypeScript | RN 0.74+ |
| 네비게이션 | @react-navigation/* | v6+ (웹 라우팅과 유사) |
| 상태 | Zustand / Redux Toolkit | 단순성 우선 Zustand |
| UI | NativeWind (Tailwind for RN) | 웹과 동일한 클래스명 사용 가능 |
| 네트워크 | axios, WebSocket | - |
| **NFC** | react-native-nfc-manager | v3.16+, NDEF 지원 |
| **BLE** | react-native-ble-plx<br>react-native-ble-manager | v3.2+ (통신)<br>v11.3+ (페어링) |
| 보안 저장소 | react-native-keychain | Keychain/Keystore 래퍼 |
| 카메라 | react-native-vision-camera | v3+ |
| 백그라운드 | react-native-background-actions | BLE/NFC 챌린지 응답 |
| 테스트 | Jest, Detox, ESLint/Prettier | 단위/E2E/린트 |

### NFC/BLE 구현 상세

**NFC (react-native-nfc-manager)**
- NDEF 읽기/쓰기 지원 (iOS/Android)
- iOS: 포어그라운드 읽기 (백그라운드 제약 회피)
- Android: 모든 NFC 기술 지원
- 챌린지-응답: NDEF 페이로드에 서명 데이터 전달

**BLE (react-native-ble-plx + react-native-ble-manager)**
- `ble-plx`: 스캔, 연결, 특성 읽기/쓰기, 알림
- `ble-manager`: 디바이스 페어링 (`createBond()`)
- 백그라운드: iOS 지원, Android 제약 (포어그라운드 서비스 권장)
- 보안: 앱 레벨 암호화 (디바이스 키 서명)

**제약사항 & 완화**
- iOS NFC 백그라운드 제한 → 앱 활성화 시 읽기
- Android BLE 백그라운드 제한 → 포어그라운드 서비스 + 알림
- 페어링 없는 BLE → `ble-manager`로 bonding 처리

---

## 13. 화면 구조

### 구현 완료 (웹 프로토타입)
1. ✅ **로그인** (`/login`) - 더미 인증, 그라디언트 배경
2. ✅ **홈** (`/`) - 예약 현황, MFA 상태, 빠른 액션
3. ✅ **대여 목록** (`/rentals`) - 검색, 필터, 차량 카드
4. ✅ **예약 상세** (`/rental/:id`) - 타임라인, 취소 버튼, MFA 안내
5. ✅ **사전 등록** (`/register`) - 3단계 (셀카→체형→NFC/BT), Progress bar
6. ✅ **제어** (`/control`) - MFA 게이트, 시트/미러 조정, 로그
7. ✅ **설정** (`/settings`) - 메뉴 목록

### React Native 앱에 추가 필요
8. ⬜ 온보딩/권한 요청 화면
9. ⬜ 실시간 알림/로그 화면 (WebSocket)
10. ⬜ 프로필 관리 상세 (CRUD)
11. ⬜ 디바이스 정보 상세 (NFC/BT)
12. ⬜ 개인정보/약관 화면

---

## 14. 테스트 시나리오

- MFA: 3단계 순차 검증 (스마트키·얼굴·NFC/BT), 각 단계 실패 시 재시도
- 제어 안전: 주행 중·도어잠금·시트 점유 조합 시 비활성
- 초기화: 주행 완료·시트 이탈 이벤트 → 세션 종료·UI 리셋
- 오프라인: 사전 등록 데이터 캐시, MFA 불가 안내

---

## 15. 마일스톤

| 단계 | 기간 | 내용 | 상태 |
|------|------|------|------|
| M0 | 1주 | 웹 프로토타입 (UI/UX 검증) | ✅ 완료 |
| M1 | 2주 | RN 프로젝트 셋업·Auth·대여 기본·사전 등록 UI | ⬜ 대기 |
| M2 | 3주 | NFC/BT 하드웨어 연동·프로필 CRUD·상태 푸시 | ⬜ 대기 |
| M3 | 3주 | 서버 API 연동·제어·안전 조건·MFA 테스트 | ⬜ 대기 |
| M4 | 2주 | QA·보안·크래시 프리 99.5%·베타 | ⬜ 대기 |

**M0 완료 사항 (웹 프로토타입)**:
- 전체 화면 플로우 구현 및 검증
- 디자인 시스템 확립 (색상, 그라디언트, 애니메이션)
- 컴포넌트 라이브러리 검증 (shadcn/ui → RN 전환 계획)
- UX 패턴 확인 (Bottom Tab, 3단계 등록, MFA 게이트 등)

---

## 16. 오픈 이슈

- CAN 메시지 상세 페이로드 (얼굴 좌표값, NFC 8byte 포맷)
- 차종 보정 테이블 (좌석/미러 축·범위·단위)
- 개인정보 보관/폐기 정책 문서화
- BLE fallback 프로토콜 (주파수·페어링·암호화)
- iOS NFC 백그라운드 읽기 vs 포어그라운드 UX 결정
- Android BLE 백그라운드 포어그라운드 서비스 알림 문구