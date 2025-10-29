# HYPERMOB 앱 시나리오 기반 API 설계 및 개발 계획

**날짜:** 2025-10-27
**버전:** 1.0
**목적:** 실제 사용자 시나리오를 완전히 구현하기 위한 API 설계 및 개발 계획

---

## 📋 목차

1. [시나리오 개요](#시나리오-개요)
2. [필요한 API 엔드포인트](#필요한-api-엔드포인트)
3. [데이터베이스 설계](#데이터베이스-설계)
4. [S3 스토리지 구조](#s3-스토리지-구조)
5. [시나리오별 상세 플로우](#시나리오별-상세-플로우)
6. [개발 우선순위](#개발-우선순위)
7. [구현 계획](#구현-계획)

---

## 시나리오 개요

### 시나리오 1: 사용자 등록 (User Registration)
개인정보 입력 → 얼굴 촬영 → 전신 촬영 → 맞춤형 프로필 생성

### 시나리오 2: 차량 대여 (Vehicle Rental)
차량 목록 조회 → 차량 선택 → 대여 신청 → 승인 대기 → 승인 완료

### 시나리오 3: 차량 인수 (Vehicle Handover with MFA)
차량 선택 → MFA 인증 시작 → NFC/BLE/Face 인증 → 인수 완료 → 제어 화면

---

## 필요한 API 엔드포인트

### 🆕 신규 엔드포인트 (시나리오 기반)

#### 1. 사용자 등록 관련

| 엔드포인트 | 메서드 | 설명 | 상태 |
|-----------|--------|------|------|
| `/users` | POST | 사용자 개인정보 등록 | 🆕 신규 |
| `/facedata/{user_id}` | POST | 얼굴 좌표 데이터 생성 및 S3 저장 | 🆕 신규 |
| `/measure/{user_id}` | POST | 전신 사진으로 체형 데이터 생성 | 🆕 신규 |
| `/recommend/{user_id}` | POST | 맞춤형 차량 세팅 생성 | 🆕 신규 |
| `/users/{user_id}/profile` | GET | 사용자 프로필 상태 조회 | ✅ 기존 |

#### 2. 차량 대여 관련

| 엔드포인트 | 메서드 | 설명 | 상태 |
|-----------|--------|------|------|
| `/vehicles` | GET | 차량 목록 조회 (위치 기반 정렬) | ✅ 기존 |
| `/vehicles/{car_id}` | GET | 특정 차량 상세 조회 | ✅ 기존 |
| `/bookings` | POST | 차량 대여 신청 | ✅ 기존 |
| `/bookings/{booking_id}` | GET | 대여 신청 상태 조회 | ✅ 기존 |
| `/bookings/user/{user_id}` | GET | 내 대여 목록 조회 | 🆕 신규 |

#### 3. MFA 인증 관련 (폴링용)

| 엔드포인트 | 메서드 | 설명 | 상태 |
|-----------|--------|------|------|
| `/auth/session/{session_id}/status` | GET | 인증 세션 상태 폴링 | 🆕 신규 |
| `/auth/session` | GET | 인증 세션 시작 | ✅ 기존 |
| `/auth/result` | POST | 차량에서 인증 결과 보고 | ✅ 기존 |

---

## 데이터베이스 설계

### 기존 테이블 (유지)

#### 1. users 테이블
```sql
CREATE TABLE users (
    user_id VARCHAR(16) PRIMARY KEY,
    face_id VARCHAR(16) NOT NULL UNIQUE,
    nfc_uid VARCHAR(14) DEFAULT NULL UNIQUE,
    face_image_path VARCHAR(512) DEFAULT NULL,  -- S3 경로
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
```

#### 2. cars 테이블
```sql
CREATE TABLE cars (
    car_id VARCHAR(16) PRIMARY KEY,
    plate_number VARCHAR(20) NOT NULL UNIQUE,
    model VARCHAR(100) NOT NULL,
    status ENUM('available', 'rented', 'maintenance') DEFAULT 'available',
    location_lat DECIMAL(10, 8) NOT NULL,      -- 위도
    location_lng DECIMAL(11, 8) NOT NULL,      -- 경도
    location_address VARCHAR(255),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
```

#### 3. bookings 테이블
```sql
CREATE TABLE bookings (
    booking_id VARCHAR(16) PRIMARY KEY,
    user_id VARCHAR(16) NOT NULL,
    car_id VARCHAR(16) NOT NULL,
    start_time TIMESTAMP NOT NULL,
    end_time TIMESTAMP NOT NULL,
    status ENUM('pending', 'approved', 'active', 'completed', 'cancelled') DEFAULT 'pending',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id),
    FOREIGN KEY (car_id) REFERENCES cars(car_id)
);
```

### 🆕 신규 테이블

#### 4. face_datas 테이블 (얼굴 좌표 데이터)
```sql
CREATE TABLE face_datas (
    face_data_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id VARCHAR(16) NOT NULL,
    s3_url VARCHAR(512) NOT NULL COMMENT '얼굴 좌표 txt 파일 S3 URL',
    landmark_count INT DEFAULT 68 COMMENT '얼굴 랜드마크 개수',
    confidence_score DECIMAL(5, 4) COMMENT '인식 신뢰도 (0.0000~1.0000)',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id)
) ENGINE=InnoDB COMMENT='얼굴 랜드마크 좌표 데이터';
```

#### 5. body_datas 테이블 (체형 데이터)
```sql
CREATE TABLE body_datas (
    body_data_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id VARCHAR(16) NOT NULL,
    height DECIMAL(5, 2) COMMENT '키 (cm)',
    shoulder_width DECIMAL(5, 2) COMMENT '어깨 너비 (cm)',
    torso_length DECIMAL(5, 2) COMMENT '상체 길이 (cm)',
    leg_length DECIMAL(5, 2) COMMENT '다리 길이 (cm)',
    arm_length DECIMAL(5, 2) COMMENT '팔 길이 (cm)',
    body_image_s3_url VARCHAR(512) COMMENT '전신 사진 S3 URL',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id)
) ENGINE=InnoDB COMMENT='사용자 체형 데이터';
```

#### 6. user_profiles 테이블 (맞춤형 차량 세팅)
```sql
CREATE TABLE user_profiles (
    profile_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id VARCHAR(16) NOT NULL,
    seat_position JSON COMMENT '시트 위치 {"forward": 50, "height": 30, "recline": 20}',
    mirror_position JSON COMMENT '미러 위치 {"left": {...}, "right": {...}, "rear": {...}}',
    steering_position JSON COMMENT '스티어링 위치 {"height": 40, "depth": 50}',
    climate_preference JSON COMMENT '선호 온도 {"temp": 22, "fan": 3, "mode": "auto"}',
    status ENUM('generating', 'completed', 'failed') DEFAULT 'generating',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_status (status)
) ENGINE=InnoDB COMMENT='사용자 맞춤형 차량 세팅';
```

#### 7. auth_sessions 테이블 (MFA 인증 세션) - 기존 테이블 확장
```sql
CREATE TABLE auth_sessions (
    session_id VARCHAR(32) PRIMARY KEY,
    booking_id VARCHAR(16) NOT NULL,
    user_id VARCHAR(16) NOT NULL,
    car_id VARCHAR(16) NOT NULL,
    nfc_verified BOOLEAN DEFAULT FALSE,
    ble_verified BOOLEAN DEFAULT FALSE,
    face_verified BOOLEAN DEFAULT FALSE,
    status ENUM('pending', 'in_progress', 'completed', 'failed') DEFAULT 'pending',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NOT NULL COMMENT '세션 만료 시간 (생성 + 10분)',
    FOREIGN KEY (booking_id) REFERENCES bookings(booking_id),
    FOREIGN KEY (user_id) REFERENCES users(user_id),
    FOREIGN KEY (car_id) REFERENCES cars(car_id),
    INDEX idx_session_status (session_id, status),
    INDEX idx_booking (booking_id),
    INDEX idx_expires (expires_at)
) ENGINE=InnoDB COMMENT='MFA 인증 세션';
```

---

## S3 스토리지 구조

### 버킷 구조

```
hypermob-user-data/
├── face-landmarks/              # 얼굴 랜드마크 좌표
│   └── {user_id}/
│       └── {timestamp}_landmarks.txt
│
├── face-images/                 # 얼굴 사진 (원본)
│   └── {user_id}/
│       └── {timestamp}_face.jpg
│
└── body-images/                 # 전신 사진
    └── {user_id}/
        └── {timestamp}_body.jpg
```

### S3 접근 패턴

#### 1. 얼굴 랜드마크 저장
```
버킷: hypermob-user-data
키: face-landmarks/{user_id}/2025-10-27-123456_landmarks.txt
URL: https://hypermob-user-data.s3.us-east-1.amazonaws.com/face-landmarks/{user_id}/...
```

#### 2. 전신 사진 저장
```
버킷: hypermob-user-data
키: body-images/{user_id}/2025-10-27-123456_body.jpg
URL: https://hypermob-user-data.s3.us-east-1.amazonaws.com/body-images/{user_id}/...
```

### S3 권한 설정
- Lambda 함수: PutObject, GetObject 권한
- 앱: 서버에서 발급한 Presigned URL로만 접근
- 만료 시간: 업로드용 5분, 다운로드용 1시간

---

## 시나리오별 상세 플로우

### 시나리오 1: 사용자 등록

#### 1.1 개인정보 등록
```
[App] POST /users
Body: {
  "name": "홍길동",
  "email": "hong@example.com",
  "phone": "010-1234-5678"
}

[Server]
1. 개인정보 검증
2. user_id 생성 (UUID 기반 16자)
3. users 테이블에 INSERT
4. user_id 반환

Response: {
  "user_id": "U1234567890ABCDE",
  "status": "created"
}
```

#### 1.2 얼굴 촬영 및 랜드마크 추출
```
[App]
1. CameraX로 얼굴 촬영
2. 촬영된 이미지를 Base64 인코딩 또는 Multipart로 전송

POST /facedata/{user_id}
Content-Type: multipart/form-data
Body: {
  "image": (binary data)
}

[Server - Lambda]
1. 이미지 수신 및 검증
2. OpenCV/Dlib으로 얼굴 랜드마크 추출
3. 좌표 데이터를 txt 파일로 생성
   예: "x1,y1\nx2,y2\n..."
4. S3에 업로드
   - 버킷: hypermob-user-data
   - 키: face-landmarks/{user_id}/{timestamp}_landmarks.txt
5. S3 URL을 face_datas 테이블에 저장
6. users 테이블의 face_id 업데이트

Response: {
  "face_data_id": 123,
  "s3_url": "https://...",
  "landmark_count": 68,
  "confidence_score": 0.9523
}
```

#### 1.3 전신 촬영 및 체형 데이터 생성
```
[App]
1. CameraX로 전신 촬영 (가이드라인 표시)
2. 이미지 전송

POST /measure/{user_id}
Content-Type: multipart/form-data
Body: {
  "image": (binary data)
}

[Server - Lambda]
1. 이미지 수신
2. S3에 원본 저장
   - 키: body-images/{user_id}/{timestamp}_body.jpg
3. OpenPose 또는 MediaPipe로 체형 측정
   - 키포인트 추출 (어깨, 골반, 무릎, 발목 등)
   - 비율 계산으로 실제 치수 추정
4. body_datas 테이블에 저장

Response: {
  "body_data_id": 456,
  "height": 175.5,
  "shoulder_width": 45.2,
  "torso_length": 52.3,
  "leg_length": 85.4,
  "arm_length": 72.1
}
```

#### 1.4 맞춤형 프로필 생성
```
[App]
1. "개인 프로필을 생성 중입니다" 로딩 화면 표시
2. 백그라운드에서 POST 요청

POST /recommend/{user_id}

[Server - Lambda]
1. face_datas, body_datas 조회
2. 알고리즘으로 최적 차량 세팅 계산
   - 키 → 시트 전후 위치
   - 팔 길이 → 스티어링 위치
   - 상체 길이 → 시트 높이
   - 어깨 너비 → 미러 각도
3. user_profiles 테이블에 저장 (status: 'completed')

Response: {
  "profile_id": 789,
  "status": "completed",
  "seat_position": {"forward": 50, "height": 30, "recline": 20},
  "mirror_position": {...},
  "steering_position": {"height": 40, "depth": 50},
  "climate_preference": {"temp": 22, "fan": 3}
}

[App]
1. 200 OK 수신 시 로딩 종료
2. "프로필 생성 완료!" 메시지
3. 메인 화면으로 이동
```

---

### 시나리오 2: 차량 대여

#### 2.1 차량 목록 조회 (위치 기반 정렬)
```
[App]
1. 현재 위치 권한 요청
2. GPS로 현재 위치 획득
   - 위도: 37.5665
   - 경도: 126.9780

GET /vehicles?lat=37.5665&lng=126.9780&status=available

[Server - Lambda]
1. cars 테이블에서 status='available' 조회
2. Haversine 공식으로 거리 계산
   distance = 2 * R * asin(sqrt(
     sin²((lat2-lat1)/2) +
     cos(lat1) * cos(lat2) * sin²((lng2-lng1)/2)
   ))
3. 거리순 정렬 (ASC)
4. 응답

Response: {
  "vehicles": [
    {
      "car_id": "C001",
      "model": "Tesla Model 3",
      "plate_number": "12가3456",
      "location_lat": 37.5660,
      "location_lng": 126.9775,
      "location_address": "서울 중구 명동",
      "distance_km": 0.52,
      "status": "available"
    },
    {
      "car_id": "C002",
      "model": "Hyundai Ioniq 5",
      "distance_km": 1.23,
      ...
    }
  ]
}

[App]
1. 리스트 표시 (거리 순)
2. 각 항목에 "거리: 0.5km" 표시
```

#### 2.2 차량 선택 및 대여 신청
```
[App]
1. 사용자가 차량 선택 (예: C001)
2. 대여 페이지 이동
   - 차량 상세 정보 표시
   - 대여 시작/종료 시간 선택

POST /bookings
Body: {
  "user_id": "U1234567890ABCDE",
  "car_id": "C001",
  "start_time": "2025-10-27T10:00:00Z",
  "end_time": "2025-10-27T18:00:00Z"
}

[Server - Lambda]
1. 차량 availability 확인
2. 시간 중복 체크
3. bookings 테이블에 INSERT
   - status: 'pending'
4. 차량 상태 변경 (cars.status = 'rented')

Response: {
  "booking_id": "B001",
  "status": "pending",
  "message": "대여 신청이 완료되었습니다. 승인 대기 중입니다."
}

[App]
1. "대여 신청 완료" 페이지 표시
2. 메인 화면 복귀
```

#### 2.3 대여 승인 및 내 대여 목록 조회
```
[App - 메인 화면]
1. 주기적으로 내 대여 목록 조회

GET /bookings/user/{user_id}

[Server]
SELECT * FROM bookings WHERE user_id = ? ORDER BY created_at DESC

Response: {
  "bookings": [
    {
      "booking_id": "B001",
      "car_id": "C001",
      "car_model": "Tesla Model 3",
      "plate_number": "12가3456",
      "status": "approved",  // pending → approved (관리자 승인)
      "start_time": "2025-10-27T10:00:00Z"
    }
  ]
}

[App]
1. status='approved' 인 차량 표시
2. "승인됨" 뱃지 표시
3. "MFA 인증 시작" 버튼 활성화
```

---

### 시나리오 3: 차량 인수 (MFA 인증)

#### 3.1 MFA 인증 시작
```
[App]
1. 승인된 차량 선택
2. "MFA 인증 시작" 버튼 클릭

POST /auth/session
Body: {
  "booking_id": "B001",
  "user_id": "U1234567890ABCDE",
  "car_id": "C001"
}

[Server]
1. booking 유효성 검증
2. auth_sessions 테이블에 INSERT
   - session_id 생성
   - status: 'pending'
   - expires_at: now() + 10분
3. 세션 ID 반환

Response: {
  "session_id": "S12345678901234567890123456789AB",
  "status": "pending",
  "expires_at": "2025-10-27T10:10:00Z"
}

[App]
1. 인증 화면으로 이동
2. NFC, BLE, Face 3개 카드 표시 (모두 "대기 중...")
3. 폴링 시작 (1초마다)
```

#### 3.2 폴링으로 인증 상태 확인
```
[App - 1초마다 실행]
GET /auth/session/{session_id}/status

[Server]
SELECT
  nfc_verified,
  ble_verified,
  face_verified,
  status
FROM auth_sessions
WHERE session_id = ?

Response: {
  "session_id": "S12345...",
  "nfc_verified": true,     // 차량에서 NFC 태그 완료
  "ble_verified": false,    // 아직 미완료
  "face_verified": false,   // 아직 미완료
  "status": "in_progress"
}

[App]
1. nfc_verified=true → NFC 카드 "인증 완료" 표시
2. ble_verified=false → BLE 카드 "인증 중..." 표시
3. face_verified=false → Face 카드 "대기 중..." 표시
4. 계속 폴링
```

#### 3.3 차량에서 인증 수행 (백엔드)
```
[Car System]
1. NFC 태그 감지
   POST /auth/nfc/verify
   Body: {"session_id": "S12345...", "nfc_uid": "04A1B2C3D4E5F6"}
   → DB: UPDATE auth_sessions SET nfc_verified=TRUE

2. BLE 연결 감지
   POST /auth/ble/verify
   Body: {"session_id": "S12345...", "ble_hash": "..."}
   → DB: UPDATE auth_sessions SET ble_verified=TRUE

3. 얼굴 인식 완료
   POST /auth/face/verify
   Body: {"session_id": "S12345...", "face_id": "F1A2B3C4D5E6F708"}
   → DB: UPDATE auth_sessions SET face_verified=TRUE

4. 모두 완료 시
   → DB: UPDATE auth_sessions SET status='completed'
```

#### 3.4 인증 완료 및 제어 화면 이동
```
[App - 폴링 결과]
Response: {
  "nfc_verified": true,
  "ble_verified": true,
  "face_verified": true,
  "status": "completed"
}

[App]
1. 3개 카드 모두 "인증 완료" 표시
2. "인수 완료!" 화면 전환 (3초간 표시)
3. 3초 후 제어 화면으로 자동 이동
   - 시트/미러/스티어링 제어 UI
   - user_profiles에서 맞춤 설정 로드
```

---

## 개발 우선순위

### Phase 1: 필수 기능 (MVP)
1. ✅ **사용자 등록 API**
   - POST /users
   - POST /facedata/{user_id}
   - POST /measure/{user_id}
   - POST /recommend/{user_id}

2. ✅ **차량 대여 API**
   - GET /vehicles (위치 기반)
   - GET /bookings/user/{user_id}

3. ✅ **MFA 인증 폴링 API**
   - GET /auth/session/{session_id}/status

### Phase 2: 고급 기능
4. 🔄 **프로필 최적화**
   - 머신러닝 기반 세팅 추천
   - 사용자 피드백 반영

5. 🔄 **실시간 알림**
   - WebSocket 또는 Firebase Cloud Messaging
   - 대여 승인 알림
   - 인증 완료 알림

### Phase 3: 개선
6. 🔄 **캐싱 및 성능**
   - Redis 캐싱
   - S3 CloudFront CDN

---

## 구현 계획

### Week 1: 데이터베이스 및 S3 설정
- [ ] MySQL 스키마 생성 (신규 테이블 4개)
- [ ] S3 버킷 생성 및 권한 설정
- [ ] Lambda Layer 업데이트 (이미지 처리 라이브러리)

### Week 2: 사용자 등록 API
- [ ] POST /users Lambda 함수
- [ ] POST /facedata/{user_id} Lambda 함수
  - OpenCV/Dlib 통합
  - S3 업로드 로직
- [ ] POST /measure/{user_id} Lambda 함수
  - MediaPipe 통합
  - 체형 측정 알고리즘
- [ ] POST /recommend/{user_id} Lambda 함수
  - 추천 알고리즘 구현

### Week 3: 차량 대여 API
- [ ] GET /vehicles 수정 (위치 기반 정렬)
- [ ] GET /bookings/user/{user_id} Lambda 함수
- [ ] 차량 승인 프로세스 구현 (관리자 페이지 또는 자동)

### Week 4: MFA 폴링 API
- [ ] GET /auth/session/{session_id}/status Lambda 함수
- [ ] auth_sessions 테이블 업데이트 로직
- [ ] 차량 측 인증 API 연동 테스트

### Week 5: Android 앱 통합
- [ ] 사용자 등록 화면 구현
- [ ] 차량 목록 화면 구현
- [ ] MFA 인증 폴링 화면 구현
- [ ] 제어 화면 구현

### Week 6: 테스트 및 배포
- [ ] 통합 테스트
- [ ] Lambda 배포
- [ ] Android 앱 빌드 및 테스트

---

## API 엔드포인트 전체 목록

### 신규 API (8개)

| No | 엔드포인트 | 메서드 | 설명 |
|----|-----------|--------|------|
| 1 | `/users` | POST | 사용자 개인정보 등록 |
| 2 | `/facedata/{user_id}` | POST | 얼굴 랜드마크 추출 및 S3 저장 |
| 3 | `/measure/{user_id}` | POST | 체형 데이터 측정 |
| 4 | `/recommend/{user_id}` | POST | 맞춤형 프로필 생성 |
| 5 | `/bookings/user/{user_id}` | GET | 내 대여 목록 조회 |
| 6 | `/auth/session/{session_id}/status` | GET | MFA 인증 상태 폴링 |
| 7 | `/auth/nfc/verify` | POST | NFC 인증 (차량용) |
| 8 | `/auth/ble/verify` | POST | BLE 인증 (차량용) |

### 기존 API (유지/수정)

| No | 엔드포인트 | 메서드 | 수정 사항 |
|----|-----------|--------|----------|
| 1 | `/vehicles` | GET | 위치 기반 정렬 추가 |
| 2 | `/vehicles/{car_id}` | GET | 유지 |
| 3 | `/bookings` | POST | 유지 |
| 4 | `/bookings/{booking_id}` | GET | 유지 |
| 5 | `/auth/session` | POST | session_id 반환 추가 |
| 6 | `/auth/result` | POST | 유지 |

**총 14개 엔드포인트**

---

## 참고 문서

- [현재 OpenAPI 스펙](../api/openapi.yaml)
- [데이터베이스 스키마](../../database/scripts/schema.sql)
- [Android 앱 아키텍처](../../android/ARCHITECTURE.md)
- [Lambda 배포 가이드](../../scripts/docs/DEPLOYMENT_GUIDE.md)

---

**작성자:** HYPERMOB Development Team
**최종 수정:** 2025-10-27
**버전:** 1.0
**상태:** 설계 완료, 구현 대기
