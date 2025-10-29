# HYPERMOB Vehicle Control Lambda

차량 설정값 적용 및 제어를 담당하는 Lambda 함수입니다.

## 엔드포인트

### POST /vehicles/{car_id}/settings/apply
사용자 프로필을 차량에 자동 적용

**Request Body:**
```json
{
  "user_id": "U12345ABCDEF",
  "settings": {
    "seat_position": 45,
    "seat_angle": 15,
    "seat_front_height": 40,
    "seat_rear_height": 42,
    "mirror_left_yaw": 150,
    "mirror_left_pitch": 10,
    "mirror_right_yaw": 210,
    "mirror_right_pitch": 12,
    "mirror_room_yaw": 180,
    "mirror_room_pitch": 8,
    "wheel_position": 90,
    "wheel_angle": 20
  }
}
```

**Response:**
```json
{
  "status": "success",
  "data": {
    "car_id": "CAR001",
    "applied_settings": {
      "seat_position": { "status": "applied", "value": 45, "timestamp": "2025-10-27T14:05:32Z" },
      "seat_angle": { "status": "applied", "value": 15, "timestamp": "2025-10-27T14:05:32Z" },
      ...
    },
    "timestamp": "2025-10-27T14:05:32Z"
  }
}
```

### POST /vehicles/{car_id}/settings/manual
수동 조작 기록

**Request Body:**
```json
{
  "user_id": "U12345ABCDEF",
  "settings": {
    "seat_position": 50,
    "mirror_left_yaw": 155
  }
}
```

**Response:**
```json
{
  "status": "success",
  "message": "수동 조작이 기록되었습니다",
  "data": {
    "car_id": "CAR001",
    "user_id": "U12345ABCDEF",
    "timestamp": "2025-10-27T14:10:00Z"
  }
}
```

### GET /vehicles/{car_id}/settings/current
현재 차량 설정값 조회

**Response:**
```json
{
  "status": "success",
  "data": {
    "car_id": "CAR001",
    "user_id": "U12345ABCDEF",
    "settings": {
      "seat_position": 45,
      "seat_angle": 15,
      ...
    },
    "adjustment_type": "auto",
    "timestamp": "2025-10-27T14:05:32Z"
  }
}
```

### GET /vehicles/{car_id}/settings/history
차량 설정 히스토리 조회

**Query Parameters:**
- `user_id` (optional): 특정 사용자 필터
- `limit` (optional): 결과 개수 제한 (기본: 10)

**Response:**
```json
{
  "status": "success",
  "data": {
    "car_id": "CAR001",
    "history": [
      {
        "id": 123,
        "user_id": "U12345ABCDEF",
        "car_id": "CAR001",
        "settings": {...},
        "adjustment_type": "auto",
        "timestamp": "2025-10-27T14:05:32Z"
      }
    ],
    "count": 10
  }
}
```

## 설정 범위 검증

### 시트 설정:
- `seat_position`: 0-100
- `seat_angle`: 0-45
- `seat_front_height`: 0-100
- `seat_rear_height`: 0-100

### 미러 설정:
- `mirror_*_yaw`: 0-360
- `mirror_*_pitch`: -30 ~ 30

### 핸들 설정:
- `wheel_position`: 0-100
- `wheel_angle`: 0-45

## 차량 인터페이스

`vehicle-interface.js` 모듈에서 실제 차량 제어를 담당합니다.

현재는 **모의 구현(Mock)**이며, 실제 배포 시 다음으로 교체 필요:
- CAN bus 통신
- 차량 제어 API
- IoT Gateway

## 환경 변수

- `DB_HOST`: MySQL 호스트
- `DB_USER`: MySQL 사용자
- `DB_PASSWORD`: MySQL 비밀번호
- `DB_NAME`: 데이터베이스 이름
- `DB_PORT`: MySQL 포트 (기본: 3306)
- `NODE_ENV`: 환경 (development/production)
