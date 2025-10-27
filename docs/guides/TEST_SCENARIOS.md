# HYPERMOB Platform AI - 테스트 시나리오

## 개요

[test_all_endpoints.ps1](test_all_endpoints.ps1) 스크립트는 전체 API를 자동으로 테스트하며, **Measure → Recommend 연계 워크플로우**를 포함합니다.

## 테스트 목록

### Test 1: Health Check
- **엔드포인트**: `GET /health
- **목적**: API Gateway 및 기본 라우팅 확인
- **예상 결과**: `{"status": "healthy"}
- **성공 조건**: 응답 시간 < 1초

### Test 2: Recommend API - Valid Request
- **엔드포인트**: `POST /recommend
- **목적**: Recommend API 정상 동작 확인
- **입력**:
  ```json
  {
    "height": 175.0,
    "upper_arm": 31.0,
    "forearm": 26.0,
    "thigh": 51.0,
    "calf": 36.0,
    "torso": 61.0,
    "user_id": "test_user_powershell"
  }
  ```
- **예상 결과**: 12개 필드 (seat_position, seat_angle, 등) 반환
- **성공 조건**:
  - 응답 시간 < 5초
  - 모든 필수 필드 존재

### Test 3: Recommend API - Error Handling (Missing Fields)
- **엔드포인트**: `POST /recommend
- **목적**: 필수 필드 누락 시 에러 처리 확인
- **입력**: `{"height": 175.0}` (필수 필드 누락)
- **예상 결과**: `400 Bad Request
- **성공 조건**: 적절한 에러 메시지 반환

### Test 4: Recommend API - Error Handling (Invalid Values)
- **엔드포인트**: `POST /recommend
- **목적**: 잘못된 값에 대한 에러 처리 확인
- **입력**: `{"height": -10, ...}` (음수 키)
- **예상 결과**: `400 Bad Request
- **성공 조건**: 적절한 에러 메시지 반환

### Test 5: Measure API
- **엔드포인트**: `POST /measure
- **목적**: 신체 측정 API 정상 동작 확인
- **전제 조건**: `s3://hypermob-images/test/full_body.jpg` 존재
- **입력**:
  ```json
  {
    "image_url": "s3://hypermob-images/test/full_body.jpg",
    "height_cm": 177.7,
    "user_id": "test_user_powershell"
  }
  ```
- **예상 결과**:
  ```json
  {
    "status": "success",
    "data": {
      "upper_arm": 32.5,
      "forearm": 26.8,
      "thigh": 45.2,
      "calf": 38.9,
      "torso": 61.5,
      "height": 177.7,
      "output_image_url": "https://...",
      "user_id": "test_user_powershell"
    }
  }
  ```
- **검증 항목**:
  - ✅ Recommend API 필수 필드 모두 포함 (upper_arm, forearm, thigh, calf, torso, height)
  - ✅ 측정 결과 이미지 URL
- **성공 조건**:
  - 응답 시간 < 30초
  - 모든 필드 존재

### **Test 6: Measure → Recommend Integration** ⭐ (NEW)
- **엔드포인트**: `POST /measure` → `POST /recommend
- **목적**: **전체 워크플로우 검증** (신체 측정 → 차량 설정 추천)
- **동작 방식**:
  1. Test 5에서 얻은 Measure 결과를 자동으로 Recommend API에 전달
  2. 필드명 호환성 검증
  3. 전체 프로세스 성공 확인

- **입력 (자동)**: Test 5의 Measure 응답 데이터
  ```json
  {
    "height": 177.7,
    "upper_arm": 32.5,
    "forearm": 26.8,
    "thigh": 45.2,
    "calf": 38.9,
    "torso": 61.5,
    "user_id": "test_user_powershell"
  }
  ```

- **예상 결과**:
  ```json
  {
    "status": "success",
    "data": {
      "seat_position": 45,
      "seat_angle": 105,
      "seat_front_height": 52,
      "seat_rear_height": 48,
      "mirror_left_yaw": 35,
      "mirror_left_pitch": 15,
      "mirror_right_yaw": 145,
      "mirror_right_pitch": 15,
      "mirror_room_yaw": 90,
      "mirror_room_pitch": 10,
      "wheel_position": 60,
      "wheel_angle": 45
    },
    "user_id": "test_user_powershell"
  }
  ```

- **검증 항목**:
  - ✅ Measure 결과가 Recommend API 요청 형식과 일치
  - ✅ 필드명 변환 불필요 (직접 전달 가능)
  - ✅ 12개 차량 설정 필드 모두 반환
  - ✅ 전체 워크플로우 성공

- **성공 조건**:
  - 응답 시간 < 10초
  - 모든 차량 설정 필드 존재

## 실행 방법

### 1. 기본 테스트 (Test 1-4 만 실행)

테스트 이미지 없이 실행 가능:

```powershell
.\test_all_endpoints.ps1
```

**예상 출력:**
```
=========================================
 Test 1: Health Check
=========================================
[PASS] Health check passed
[PASS] Response time < 1s

=========================================
 Test 2: Recommend API - Valid Request
=========================================
[PASS] Status is 'success'
[PASS] All required fields present
[PASS] Response time < 5s

=========================================
 Test Summary
=========================================
Total Tests: 4
Passed:      4
Failed:      0

All tests passed!
```

### 2. 전체 테스트 (Test 1-6 실행)

**전제 조건: S3에 테스트 이미지 업로드**

```powershell
# 테스트 이미지 업로드 (한 번만 실행)
aws s3 cp your_full_body_image.jpg s3://hypermob-images/test/full_body.jpg --region ap-northeast-2

# 이미지 확인
aws s3 ls s3://hypermob-images/test/full_body.jpg --region ap-northeast-2

# 전체 테스트 실행
.\test_all_endpoints.ps1
```

**예상 출력:**
```
=========================================
 Test 5: Measure API (Optional)
=========================================
[TEST] Testing POST /measure
[INFO] Response received in 8.5s
{
  "status": "success",
  "data": {
    "upper_arm": 32.5,
    "forearm": 26.8,
    "thigh": 45.2,
    "calf": 38.9,
    "torso": 61.5,
    "height": 177.7,
    "output_image_url": "https://...",
    "user_id": "test_user_powershell"
  }
}
[PASS] All required fields present for Recommend API
[PASS] Measure API test passed
[PASS] Response time < 30s

=========================================
 Test 6: Measure → Recommend Integration
=========================================
[TEST] Using Measure API results to call Recommend API

[INFO] Measure results:
  height     = 177.7
  upper_arm  = 32.5
  forearm    = 26.8
  thigh      = 45.2
  calf       = 38.9
  torso      = 61.5

[INFO] Calling Recommend API with Measure results...
[INFO] Response received in 2.1s
{
  "status": "success",
  "data": {
    "seat_position": 45,
    "seat_angle": 105,
    ...
  }
}
[PASS] Status is 'success'
[PASS] All required fields present
[PASS] Measure → Recommend integration test passed!
[PASS] Full workflow completed successfully!
[PASS] Response time < 10s

=========================================
 Test Summary
=========================================
Total Tests: 6
Passed:      6
Failed:      0

All tests passed! API is working correctly.
```

## 배포 후 테스트 순서

### 1단계: Measure Lambda 재배포

```powershell
# Measure 함수 재배포 (개선된 측정 로직)
.\rebuild_measure.ps1
```

### 2단계: Recommend Lambda 재배포

```powershell
# Recommend 함수 재배포 (S3 모델 로딩)
.\rebuild_recommend.ps1
```

### 3단계: 전체 테스트

```powershell
# 테스트 이미지 업로드 (한 번만)
aws s3 cp your_image.jpg s3://hypermob-images/test/full_body.jpg --region ap-northeast-2

# 전체 테스트 실행
.\test_all_endpoints.ps1
```

### 예상 결과

✅ **모든 테스트 통과 (6/6)**
- Test 1: Health Check ✅
- Test 2: Recommend API (Valid) ✅
- Test 3: Recommend API (Error: Missing Fields) ✅
- Test 4: Recommend API (Error: Invalid Values) ✅
- Test 5: Measure API ✅
- Test 6: **Measure → Recommend Integration** ✅ ⭐

## 요약

- ✅ 6개 테스트 자동 실행
- ✅ **Measure → Recommend 연계 워크플로우 검증**
- ✅ 필드명 호환성 확인
- ✅ 에러 처리 검증
- ✅ 성능 벤치마킹

**전체 워크플로우 테스트로 실제 사용 시나리오를 완벽하게 검증할 수 있습니다!**
