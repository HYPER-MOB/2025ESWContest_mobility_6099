# HYPERMOB User Profile Lambda

사용자 프로필 조회 및 관리를 담당하는 Lambda 함수입니다.

## 엔드포인트

### GET /users/{user_id}/profile
사용자 프로필 조회

**Response (프로필 존재):**
```json
{
  "status": "success",
  "data": {
    "user_id": "U12345ABCDEF",
    "body_measurements": {
      "height": 175.0,
      "upper_arm": 31.0,
      "forearm": 26.0,
      "thigh": 51.0,
      "calf": 36.0,
      "torso": 61.0
    },
    "vehicle_settings": {
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
    },
    "has_profile": true,
    "created_at": "2025-10-20T10:00:00Z",
    "updated_at": "2025-10-27T09:30:00Z"
  }
}
```

**Response (프로필 없음):**
```json
{
  "status": "success",
  "data": {
    "user_id": "U12345ABCDEF",
    "has_profile": false,
    "message": "사용자 프로필이 존재하지 않습니다"
  }
}
```

### PUT /users/{user_id}/profile
사용자 프로필 생성 또는 업데이트

**Request Body:**
```json
{
  "body_measurements": {
    "height": 175.0,
    "upper_arm": 31.0,
    "forearm": 26.0,
    "thigh": 51.0,
    "calf": 36.0,
    "torso": 61.0
  },
  "vehicle_settings": {
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
    "user_id": "U12345ABCDEF",
    "body_measurements": {...},
    "vehicle_settings": {...},
    "has_profile": true,
    "action": "created"
  }
}
```

### DELETE /users/{user_id}/profile
사용자 프로필 삭제

**Response:**
```json
{
  "status": "success",
  "message": "프로필이 삭제되었습니다"
}
```

## 검증 규칙

### body_measurements 필수 필드 (모두 number):
- height
- upper_arm
- forearm
- thigh
- calf
- torso

### vehicle_settings 필수 필드 (모두 number):
- seat_position
- seat_angle
- seat_front_height
- seat_rear_height
- mirror_left_yaw
- mirror_left_pitch
- mirror_right_yaw
- mirror_right_pitch
- mirror_room_yaw
- mirror_room_pitch
- wheel_position
- wheel_angle

## 환경 변수

- `DB_HOST`: MySQL 호스트
- `DB_USER`: MySQL 사용자
- `DB_PASSWORD`: MySQL 비밀번호
- `DB_NAME`: 데이터베이스 이름
- `DB_PORT`: MySQL 포트 (기본: 3306)
