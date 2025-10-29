# MyDrive3DX Web Prototype

> 작성일: 2025-01
> 목적: React Native 모바일 앱 개발을 위한 UI/UX 검증용 웹 프로토타입

## 개요

MyDrive3DX는 **개인화 주행 플랫폼**을 위한 모바일 애플리케이션입니다. 차량 대여, MFA 사전 등록, 차량 제어 기능을 제공하며, 체형 기반 자동 프로필 적용을 통해 개인화된 주행 경험을 제공합니다.

이 웹 프로토타입은 React Native 앱 개발 전 핵심 UI/UX를 검증하기 위해 구축되었습니다.

## 핵심 기능

- **차량 대여**: 위치·차종·옵션 기반 예약 및 배차 관리
- **사전 등록**: 셀카·체형·NFC/BT 디바이스 정보 등록 (3단계)
- **MFA 인증**: 차량 측 3단계 인증 (스마트키 + 얼굴 + NFC/BT) 준비
- **차량 제어**: 시트/미러 조정 및 프로필 자동 적용
- **실시간 동기화**: MFA 상태 및 제어 피드백

## 기술 스택

| 항목 | 기술 |
|------|------|
| 코어 | React 18 + TypeScript + Vite |
| 라우팅 | React Router v6 |
| 상태 관리 | React Hooks (useState) |
| UI | shadcn/ui + Tailwind CSS |
| 알림 | Toaster + Sonner |
| 아이콘 | lucide-react |

## 시작하기

### 요구사항

- Node.js 18+ & npm
- [nvm 설치 가이드](https://github.com/nvm-sh/nvm#installing-and-updating)

### 설치 및 실행

```sh
# 저장소 클론
git clone <YOUR_GIT_URL>

# 프로젝트 디렉토리 이동
cd web

# 의존성 설치
npm install

# 개발 서버 실행
npm run dev
```

개발 서버는 `http://localhost:8080`에서 실행됩니다.

### 빌드

```sh
# 프로덕션 빌드
npm run build

# 빌드 결과 미리보기
npm run preview
```

빌드된 파일은 `dist/` 폴더에 생성됩니다.

## 구현된 화면

### 1. 로그인 (`/login`)
- 더미 인증 (id: `dev`, pw: `dev`)
- localStorage 기반 인증 저장
- 그라디언트 배경 + 애니메이션

### 2. 홈 (`/`)
- 예약 현황 (차량, 위치, 시간, 상태)
- MFA 상태 (등록 여부, 안내)
- 빠른 액션 (대여하기, 등록하기)

### 3. 차량 대여 (`/rentals`)
- 검색 바 (지역·차종)
- 필터 (좌석·미러·스티어링)
- 차량 카드 목록 (모델명, 위치, capabilities)

### 4. 예약 상세 (`/rental/:id`)
- 예약 정보 (차량, 날짜, 시간)
- 타임라인 (requested → approved → picked)
- MFA 안내 카드
- 예약 취소 버튼

### 5. 사전 등록 (`/register`)
- **3단계 진행**: 셀카 → 체형 → NFC/BT
- Progress bar (0% → 33% → 66% → 100%)
- 단계별 완료 체크마크
- 체형 입력: 키, 앉은키, 어깨폭, 팔길이, 시야각

### 6. 차량 제어 (`/control`)
- **MFA 게이트**: 인증 전 잠금 화면
- **안전 게이트**: 주행 중 제어 차단
- 시트 조정 (슬라이드, 높이, 등받이)
- 미러 8방향 조정 (좌/우 독립)
- 프로필 자동 적용 버튼
- 실시간 로그 표시

### 7. 설정 (`/settings`)
- 메뉴 목록 (계정, 디바이스, 알림, 약관)
- 버전 정보 (v1.0.0)

## 디자인 시스템

### 색상 토큰
```css
--primary: hsl(189 85% 55%)        /* 시안 블루 */
--primary-glow: hsl(189 95% 65%)   /* 밝은 시안 */
--background: hsl(222 47% 11%)     /* 다크 배경 */
--card: hsl(222 47% 15%)           /* 카드 배경 */
--destructive: hsl(0 84% 60%)      /* 에러/경고 */
--success: hsl(142 76% 36%)        /* 성공 */
```

### 컴포넌트
- **Card**: 그라디언트 배경 + 그림자
- **Button**: `variant="hero"` (primary), `outline`, `ghost`
- **StatusBadge**: Rental (requested, approved, picked), MFA (pending, passed, failed)

### 레이아웃
- **Header**: 로고 + 로그아웃 (sticky top-0)
- **Main**: 라우트별 컨텐츠 (flex-1)
- **Bottom Tab**: 5개 탭 (홈, 대여, 등록, 제어, 설정)

## 라우팅 구조

```
/login (공개)
/ (보호된 라우트)
  ├─ /                (홈)
  ├─ /rentals         (대여 목록)
  ├─ /rental/:id      (예약 상세)
  ├─ /register        (사전 등록)
  ├─ /control         (차량 제어)
  └─ /settings        (설정)
```

- **ProtectedRoute**: localStorage 'auth' 체크, 미인증 시 `/login`으로 리다이렉트

## React Native 전환 계획

### Phase 1: 프로젝트 셋업
- React Native CLI 프로젝트 생성 (TypeScript)
- NativeWind 설정 (Tailwind for RN)
- React Navigation 설정 (Bottom Tab + Stack)
- 디자인 시스템 토큰 이식

### Phase 2: 화면 이식
- 웹 프로토타입의 7개 화면 → React Native 컴포넌트로 변환
- `<div>` → `<View>`, `<button>` → `<Pressable>`, `<input>` → `<TextInput>`

### Phase 3: 하드웨어 연동
- **카메라**: `react-native-vision-camera` (셀카)
- **NFC**: `react-native-nfc-manager` (페어링 및 챌린지-응답)
- **BLE**: `react-native-ble-plx` + `react-native-ble-manager`
- **백그라운드 서비스**: MFA NFC/BT 응답

### Phase 4: 서버 API 연동
- 인증 API (JWT)
- 대여 API (CRUD)
- 프로필 API (사전 등록)
- 제어 API (시트/미러 명령)
- WebSocket (MFA 상태, 실시간 로그)

### Phase 5: 테스트 및 배포
- Jest 단위 테스트
- Detox E2E 테스트
- iOS/Android 빌드
- 베타 배포 (TestFlight, Play Console)

## 검증된 UX 패턴

1. **3단계 진행 표시**: Progress bar + 단계별 완료 체크
2. **MFA 게이트**: 인증 전 잠금 화면 → 인증 후 제어 가능
3. **안전 게이트**: 주행 중 제어 차단 안내
4. **타임라인**: 예약 진행 상황 시각화
5. **8방향 미러 조정**: 3x3 그리드 (중앙 비활성)
6. **실시간 로그**: 폰트 mono, 시간 + 이벤트
7. **Toast 알림**: 성공/실패 피드백

## 시스템 아키텍처

```
[모바일(RN)] ↔ [API 서버] ↔ [TCU(RPi)] ↔ [DCU] ↔ [Body CAN]
                              ↕
                            [SCA(Jetson)]
```

### MFA 플로우
1. Power Seat → DCU (착좌 감지)
2. DCU → SCA (얼굴 인식 요청)
3. SCA ↔ TCU ↔ 서버 (사용자 정보 조회)
4. SCA: 스마트키 + 얼굴 + NFC/BT 검증
5. SCA → DCU (인증 결과)
6. DCU → Body CAN (프로필 적용)

**앱 역할**:
- 사전: 얼굴·체형·NFC/BT 디바이스 정보 서버 등록
- 현장: SCA의 NFC/BT 챌린지 수신 시 서명/토큰 응답 (백그라운드)

## 참고 문서

- **PRD**: [`.docs/prd.md`](.docs/prd.md) - 제품 요구사항 정의
- **Flowchart**: [`.docs/flowchart.md`](.docs/flowchart.md) - MFA 플로우 다이어그램
- **Web Summary**: [`.docs/web-prototype-summary.md`](.docs/web-prototype-summary.md) - 웹 프로토타입 상세 요약
- **Wireframe**: [`.docs/wireframe.md`](.docs/wireframe.md) - 화면 설계

## 파일 구조

```
web/
├── src/
│   ├── components/        # 공통 컴포넌트
│   │   ├── Layout.tsx    # Header + Bottom Tab
│   │   ├── StatusBadge.tsx
│   │   └── ui/           # shadcn/ui 컴포넌트
│   ├── pages/            # 라우트별 화면
│   │   ├── Home.tsx
│   │   ├── Login.tsx
│   │   ├── Rentals.tsx
│   │   ├── RentalDetail.tsx
│   │   ├── Register.tsx
│   │   ├── Control.tsx
│   │   └── Settings.tsx
│   ├── App.tsx           # 라우팅 설정
│   ├── main.tsx          # 엔트리 포인트
│   └── index.css         # 글로벌 스타일 + 디자인 토큰
├── public/               # 정적 파일
├── index.html            # HTML 템플릿
├── vite.config.ts        # Vite 설정
└── tailwind.config.ts    # Tailwind 설정
```
