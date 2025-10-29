# HYPERMOB Vehicles Lambda

차량 목록 조회 및 관리를 담당하는 Lambda 함수입니다.

## 엔드포인트

### GET /vehicles
사용 가능한 차량 목록 조회

**Query Parameters:**
- `status` (optional): 차량 상태 필터 (available, in_use, maintenance)
- `location` (optional): 위치 필터 (부분 일치)

**Response:**
```json
{
  "status": "success",
  "data": {
    "vehicles": [
      {
        "car_id": "CAR001",
        "model": "Genesis GV80",
        "status": "available",
        "location": "Seoul Gangnam Station",
        "features": ["autonomous_driving", "adaptive_seat"],
        "created_at": "2025-10-20T10:00:00Z",
        "updated_at": "2025-10-27T09:30:00Z"
      }
    ],
    "count": 1
  }
}
```

### GET /vehicles/{car_id}
특정 차량 상세 정보 조회

**Response:**
```json
{
  "status": "success",
  "data": {
    "car_id": "CAR001",
    "model": "Genesis GV80",
    "status": "available",
    "location": "Seoul Gangnam Station",
    "features": ["autonomous_driving", "adaptive_seat"]
  }
}
```

## 환경 변수

- `DB_HOST`: MySQL 호스트
- `DB_USER`: MySQL 사용자
- `DB_PASSWORD`: MySQL 비밀번호
- `DB_NAME`: 데이터베이스 이름
- `DB_PORT`: MySQL 포트 (기본: 3306)

## 배포

```bash
npm install
zip -r function.zip .
aws lambda update-function-code \
  --function-name hypermob-vehicles \
  --zip-file fileb://function.zip
```
