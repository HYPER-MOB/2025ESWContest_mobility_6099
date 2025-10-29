# HYPERMOB Bookings Lambda

차량 예약 생성 및 관리를 담당하는 Lambda 함수입니다.

## 엔드포인트

### POST /bookings
차량 예약 생성

**Request Body:**
```json
{
  "user_id": "U12345ABCDEF",
  "car_id": "CAR001",
  "start_time": "2025-10-27T14:00:00Z",
  "end_time": "2025-10-27T18:00:00Z"
}
```

**Response:**
```json
{
  "status": "success",
  "data": {
    "booking_id": "BK1730000000001",
    "user_id": "U12345ABCDEF",
    "car_id": "CAR001",
    "status": "confirmed",
    "start_time": "2025-10-27T14:00:00Z",
    "end_time": "2025-10-27T18:00:00Z",
    "created_at": "2025-10-27T10:00:00Z"
  }
}
```

### GET /bookings/{booking_id}
특정 예약 조회

**Response:**
```json
{
  "status": "success",
  "data": {
    "booking_id": "BK1730000000001",
    "user_id": "U12345ABCDEF",
    "car_id": "CAR001",
    "status": "confirmed",
    "start_time": "2025-10-27T14:00:00Z",
    "end_time": "2025-10-27T18:00:00Z"
  }
}
```

### GET /bookings?user_id={user_id}
사용자별 예약 목록 조회

**Response:**
```json
{
  "status": "success",
  "data": {
    "bookings": [...],
    "count": 5
  }
}
```

### PATCH /bookings/{booking_id}
예약 취소

**Request Body:**
```json
{
  "status": "cancelled"
}
```

## 검증 규칙

- 모든 필수 필드 존재 여부 확인
- 시간 형식 검증 (ISO 8601)
- 종료 시간 > 시작 시간
- 과거 시간 예약 불가
- 최대 예약 기간: 7일
- 예약 충돌 검사

## 환경 변수

- `DB_HOST`: MySQL 호스트
- `DB_USER`: MySQL 사용자
- `DB_PASSWORD`: MySQL 비밀번호
- `DB_NAME`: 데이터베이스 이름
- `DB_PORT`: MySQL 포트 (기본: 3306)
