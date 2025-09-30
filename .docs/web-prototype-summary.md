# MyDrive3DX Web Prototype - 구현 요약

> 작성일: 2025-01
> 목적: React Native 모바일 앱 개발을 위한 UI/UX 검증용 웹 프로토타입

## 개요

웹 프로토타입을 통해 MyDrive3DX 모바일 앱의 핵심 화면 및 플로우를 구현하고 검증했습니다. 본 프로토타입은 React Native 앱 개발 시 UI/UX 참고 자료로 활용됩니다.

## 기술 스택

| 항목 | 기술 |
|------|------|
| 프레임워크 | React 18 + TypeScript |
| 빌드 도구 | Vite |
| 라우팅 | React Router v6 |
| UI 라이브러리 | shadcn/ui + Tailwind CSS |
| 아이콘 | lucide-react |
| 알림 | Toaster + Sonner |
| 상태 관리 | React Hooks (useState) |

## 구현된 화면

### 1. 로그인 (`/login`)
- **경로**: `web/src/pages/Login.tsx`
- **특징**:
  - 중앙 정렬 레이아웃
  - 애니메이션 배경 (pulse gradient)
  - 더미 인증 (id: dev, pw: dev)
  - localStorage 기반 인증 저장
  - Toast 알림 (성공/실패)
- **컴포넌트**: Card, Input, Button
- **로그인 성공 시** → 홈(`/`)으로 리다이렉트

### 2. 홈 (`/`)
- **경로**: `web/src/pages/Home.tsx`
- **특징**:
  - 세 개의 카드 영역:
    1. 예약 현황 (차량, 위치, 시간, 상태 배지)
    2. MFA 상태 (등록 여부, 안내 메시지, CTA)
    3. 빠른 액션 (대여하기, 프로필 등록)
  - StatusBadge 컴포넌트 사용
  - Mock 데이터
- **네비게이션**:
  - 예약 상세 → `/rental/:id`
  - 사전 등록 → `/register`
  - 차량 대여 → `/rentals`

### 3. 차량 대여 목록 (`/rentals`)
- **경로**: `web/src/pages/Rentals.tsx`
- **특징**:
  - 검색 바 (지역/차종)
  - 필터 버튼 (좌석·미러·스티어링)
  - 차량 카드 목록:
    - 모델명, 위치, capabilities (Badge)
    - 대여중 상태 표시
  - 호버 효과 (border-primary)
- **Mock 데이터**: 4개 차량 (SEDAN, SUV, HATCH)

### 4. 예약 상세 (`/rental/:id`)
- **경로**: `web/src/pages/RentalDetail.tsx`
- **특징**:
  - 뒤로 가기 버튼
  - 예약 정보 카드 (차량, 위치, 날짜, 시간)
  - 타임라인 (requested → approved → picked)
  - MFA 안내 카드 (3단계 설명)
  - 예약 취소 버튼 (approved 상태)
- **컴포넌트**: StatusBadge, Timeline

### 5. 사전 등록 (`/register`)
- **경로**: `web/src/pages/Register.tsx`
- **특징**:
  - 3단계 진행: 셀카 → 체형 → NFC/BT
  - Progress bar (0%, 33%, 66%, 100%)
  - 단계별 완료 체크마크
  - 각 단계:
    1. **셀카**: 카메라 미리보기 영역, 촬영 가이드
    2. **체형**: 5개 입력 필드 (키, 앉은키, 어깨폭, 팔길이, 시야각)
    3. **NFC/BT**: 디바이스 정보 표시 (ID, 공개키, MAC, 토큰)
  - 단계 간 이동 가능
- **컴포넌트**: Progress, Input, Badge

### 6. 차량 제어 (`/control`)
- **경로**: `web/src/pages/Control.tsx`
- **특징**:
  - **MFA 게이트**: 인증 전 잠금 화면 표시
  - **안전 게이트**: 주행 중 제어 차단
  - 인증 후 제어 가능:
    - 상태 카드 (MFA passed, Safety OK)
    - 프로필 자동 적용 버튼
    - 시트 조정 (슬라이더 3개)
      - 슬라이드: -10 ~ 10
      - 높이: 0 ~ 5
      - 등받이: 90° ~ 120°
    - 미러 8방향 조정 (3x3 그리드, 좌/우 독립)
    - 실시간 로그 (폰트 mono)
- **컴포넌트**: Slider, Button (icon), StatusBadge
- **시뮬레이션**: 인증/안전 모드 토글 버튼

### 7. 설정 (`/settings`)
- **경로**: `web/src/pages/Settings.tsx`
- **특징**:
  - 메뉴 카드 목록:
    - 계정/로그인
    - 디바이스 정보 (NFC/BT)
    - 알림 설정
    - 개인정보/약관
    - 버전 정보
  - 버전 정보 표시 (v1.0.0)
- **컴포넌트**: Card, ChevronRight 아이콘

## 레이아웃 및 네비게이션

### Layout (`web/src/components/Layout.tsx`)
- **구조**:
  - Header (sticky top-0): 로고 + 로그아웃 버튼
  - Main (flex-1): 라우트별 컨텐츠
  - Bottom Tab (sticky bottom-0): 5개 탭
- **Bottom Tab**:
  1. 홈 (Home)
  2. 대여 (CalendarClock)
  3. 등록 (IdCard)
  4. 제어 (Car)
  5. 설정 (Settings)
- **활성 탭**: primary 색상 + 배경 강조

### 반응형
- `min-h-screen`: 화면 줄임 시 스크롤 가능
- `max-w-2xl` (672px): 태블릿 대응
- `container mx-auto p-4`: 중앙 정렬 + 패딩

## 디자인 시스템

### 색상 (`web/src/index.css`)
```css
--primary: hsl(189 85% 55%)        /* 시안 블루 */
--primary-glow: hsl(189 95% 65%)   /* 밝은 시안 */
--background: hsl(222 47% 11%)     /* 다크 배경 */
--card: hsl(222 47% 15%)           /* 카드 배경 */
--secondary: hsl(217 33% 22%)      /* 보조 색상 */
--destructive: hsl(0 84% 60%)      /* 에러/경고 */
--success: hsl(142 76% 36%)        /* 성공 */
--warning: hsl(38 92% 50%)         /* 주의 */
```

### 그라디언트
- `gradient-primary`: primary → primary-glow
- `gradient-card`: 카드 배경 그라디언트

### 애니메이션
- `transition-smooth`: 0.3s cubic-bezier(0.4, 0, 0.2, 1)
- `glow-primary`: box-shadow 0 0 30px primary/0.3

### 컴포넌트
- **Card**: `gradient-card` + `shadow-lg` + `border-border/50`
- **Button**:
  - `variant="hero"`: primary 배경 (구현 필요 확인)
  - `variant="outline"`: 테두리만
  - `variant="ghost"`: 배경 없음
- **Badge**: 상태별 색상 (secondary, destructive, success 등)
- **StatusBadge** (`web/src/components/StatusBadge.tsx`):
  - Rental: requested, approved, picked, returned, canceled
  - MFA: pending, passed, failed
  - Safety: ok, block

## 라우팅

### 보호된 라우트
- ProtectedRoute HOC: localStorage 'auth' 체크
- 미인증 시 `/login`으로 리다이렉트

### 라우트 구조
```
/login (Layout 제외)
/ (ProtectedRoute + Layout)
  ├─ /
  ├─ /rentals
  ├─ /rental/:id
  ├─ /register
  ├─ /control
  └─ /settings
```

## Mock 데이터

### 홈 화면
- `activeRental`: SEDAN-24, SEOUL-01, 10:00~12:00, approved
- `mfaStatus`: registered=false, status=pending

### 대여 목록
- 4개 차량 (V-001 ~ V-004)
- capabilities: seat, mirror, steering
- available: true/false

### 예약 상세
- timeline: requested (10:00) → approved (10:02) → picked (--)

### 사전 등록
- bodyMetrics: height, sitHeight, shoulderWidth, armLength, viewAngle

### 제어
- seatPosition: slide=0, height=2, backrest=105

## React Native 전환 시 고려사항

### UI 라이브러리
- **shadcn/ui** → **NativeWind** 또는 **React Native Paper**
- Tailwind 클래스명 유지 가능 (NativeWind 사용 시)

### 컴포넌트 매핑
| 웹 | React Native |
|-----|--------------|
| `<div>` | `<View>` |
| `<button>` | `<TouchableOpacity>` / `<Pressable>` |
| `<input>` | `<TextInput>` |
| `<img>` | `<Image>` |
| CSS classes | StyleSheet / NativeWind |

### 네비게이션
- React Router → `@react-navigation/native`
  - Stack Navigator (화면 전환)
  - Bottom Tab Navigator (탭바)

### 상태 관리
- useState → **Zustand** 또는 **Redux Toolkit**
- 서버 상태: **React Query** (TanStack Query)

### 하드웨어 연동
- **카메라**: `react-native-vision-camera`
- **NFC**: `react-native-nfc-manager`
- **BLE**: `react-native-ble-plx` + `react-native-ble-manager`
- **로컬 저장소**: `@react-native-async-storage/async-storage` 또는 `react-native-keychain`

### 알림
- Toast → `react-native-toast-message`
- Push Notification → `@react-native-firebase/messaging`

### WebSocket
- 실시간 MFA 상태 동기화
- 제어 명령 피드백

## 검증된 UX 패턴

1. **3단계 진행 표시**: Progress bar + 단계별 완료 체크
2. **MFA 게이트**: 인증 전 잠금 화면 → 인증 후 제어 가능
3. **안전 게이트**: 주행 중 제어 차단 안내
4. **타임라인**: 예약 진행 상황 시각화
5. **8방향 미러 조정**: 3x3 그리드 (중앙 표시)
6. **실시간 로그**: 폰트 mono, 시간 + 이벤트
7. **Toast 알림**: 성공/실패 피드백

## 다음 단계

### Phase 1: React Native 프로젝트 셋업
- [ ] React Native CLI 프로젝트 생성 (TypeScript)
- [ ] NativeWind 설정 (Tailwind for RN)
- [ ] React Navigation 설정 (Bottom Tab + Stack)
- [ ] 디자인 시스템 토큰 이식 (색상, 폰트, 간격)

### Phase 2: 화면 이식
- [ ] 로그인 화면
- [ ] 홈 화면
- [ ] 대여 목록/상세
- [ ] 사전 등록 (3단계)
- [ ] 제어 화면
- [ ] 설정 화면

### Phase 3: 하드웨어 연동
- [ ] 카메라 API 연동 (셀카)
- [ ] NFC 페어링 및 챌린지-응답
- [ ] BLE 페어링 및 통신
- [ ] 백그라운드 서비스 (MFA 응답)

### Phase 4: 서버 API 연동
- [ ] 인증 API (JWT)
- [ ] 대여 API (CRUD)
- [ ] 프로필 API (사전 등록)
- [ ] 제어 API (시트/미러 명령)
- [ ] WebSocket (MFA 상태, 실시간 로그)

### Phase 5: 테스트 및 배포
- [ ] Jest 단위 테스트
- [ ] Detox E2E 테스트
- [ ] iOS/Android 빌드 및 테스트
- [ ] 베타 배포 (TestFlight, Play Console)

## 참고 파일

- **화면**: `web/src/pages/*.tsx`
- **레이아웃**: `web/src/components/Layout.tsx`
- **컴포넌트**: `web/src/components/StatusBadge.tsx`, `web/src/components/ui/*.tsx`
- **스타일**: `web/src/index.css`
- **라우팅**: `web/src/App.tsx`
- **문서**: `.docs/prd.md`, `.docs/wireframe.md`, `.docs/flowchart.md`