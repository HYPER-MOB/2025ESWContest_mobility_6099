# Measure Lambda 함수 개선

## 개요

원본 [measure.py](measure.py)와 동일한 측정 로직으로 Lambda 함수를 개선했습니다. Recommend API가 요구하는 정확한 신체 치수 필드를 제공합니다.

## 주요 변경사항

### 1. 측정 필드 세분화

#### 이전 (문제)
```json
{
  "arm_cm": 65.2,      // 상완 + 전완 합계 (분리 불가능)
  "leg_cm": 92.4,      // 허벅지 + 종아리 합계 (분리 불가능)
  "torso_cm": 61.5
}
```

**문제점:**
- Recommend API가 요구하는 필드: `upper_arm`, `forearm`, `thigh`, `calf`, `torso`, `height
- Measure API가 반환하는 필드: `arm_cm`, `leg_cm`, `torso_cm
- **필드명 불일치 및 세분화 부족으로 Recommend API 호출 불가능**

#### 이후 (해결)
```json
{
  "upper_arm": 32.5,   // 상완 (어깨 → 팔꿈치)
  "forearm": 26.8,     // 전완 (팔꿈치 → 손목)
  "thigh": 45.2,       // 허벅지 (엉덩이 → 무릎)
  "calf": 38.9,        // 종아리 (무릎 → 발목)
  "torso": 61.5,       // 몸통 (어깨 중간 → 엉덩이 중간)
  "height": 175.0      // 키 (입력값)
}
```

**해결:**
- ✅ Recommend API 필드명과 정확히 일치
- ✅ 상완/전완, 허벅지/종아리 분리 측정
- ✅ 키 정보 포함 (Recommend API 요구사항)

### 2. 코드 변경

#### [measure/lambda_handler.py:105-130](measure/lambda_handler.py#L105-L130)

**arm_length() 함수 개선:**
```python
def arm_length(lm, side, w, h):
    """팔 길이 측정 (상완, 전완 분리 반환)"""
    SHOULDER = getattr(POSE_LM, f"{side}_SHOULDER")
    ELBOW = getattr(POSE_LM, f"{side}_ELBOW")
    WRIST = getattr(POSE_LM, f"{side}_WRIST")

    if min(lm[SHOULDER].visibility, lm[ELBOW].visibility, lm[WRIST].visibility) < 0.5:
        return None

    upper = euclid(lm[SHOULDER], lm[ELBOW], w, h)  # 상완
    lower = euclid(lm[ELBOW], lm[WRIST], w, h)     # 전완
    return upper, lower  # 튜플로 분리 반환
```

**leg_length() 함수 개선:**
```python
def leg_length(lm, side, w, h):
    """다리 길이 측정 (허벅지, 종아리 분리 반환)"""
    HIP = getattr(POSE_LM, f"{side}_HIP")
    KNEE = getattr(POSE_LM, f"{side}_KNEE")
    ANKLE = getattr(POSE_LM, f"{side}_ANKLE")

    if min(lm[HIP].visibility, lm[KNEE].visibility, lm[ANKLE].visibility) < 0.5:
        return None

    upper = euclid(lm[HIP], lm[KNEE], w, h)      # 허벅지
    lower = euclid(lm[KNEE], lm[ANKLE], w, h)    # 종아리
    return upper, lower  # 튜플로 분리 반환
```

#### [measure/lambda_handler.py:174-220](measure/lambda_handler.py#L174-L220)

**measure_body() 함수 개선:**
```python
# 팔 길이 측정 (상완, 전완 분리)
left_arm = arm_length(lm, 'LEFT', w, h)
right_arm = arm_length(lm, 'RIGHT', w, h)

upper_arm_cm = None
forearm_cm = None

if left_arm and right_arm:
    # 양쪽 팔 모두 측정 가능한 경우 평균
    left_upper, left_forearm = left_arm
    right_upper, right_forearm = right_arm
    upper_arm_cm = (left_upper + right_upper) / 2.0 * scale
    forearm_cm = (left_forearm + right_forearm) / 2.0 * scale
elif left_arm:
    # 왼팔만 측정 가능한 경우
    left_upper, left_forearm = left_arm
    upper_arm_cm = left_upper * scale
    forearm_cm = left_forearm * scale
elif right_arm:
    # 오른팔만 측정 가능한 경우
    right_upper, right_forearm = right_arm
    upper_arm_cm = right_upper * scale
    forearm_cm = right_forearm * scale

# 다리 길이 측정 (허벅지, 종아리 분리)
left_leg = leg_length(lm, 'LEFT', w, h)
right_leg = leg_length(lm, 'RIGHT', w, h)

thigh_cm = None
calf_cm = None

if left_leg and right_leg:
    # 양쪽 다리 모두 측정 가능한 경우 평균
    left_thigh, left_calf = left_leg
    right_thigh, right_calf = right_leg
    thigh_cm = (left_thigh + right_thigh) / 2.0 * scale
    calf_cm = (left_calf + right_calf) / 2.0 * scale
elif left_leg:
    # 왼쪽 다리만 측정 가능한 경우
    left_thigh, left_calf = left_leg
    thigh_cm = left_thigh * scale
    calf_cm = left_calf * scale
elif right_leg:
    # 오른쪽 다리만 측정 가능한 경우
    right_thigh, right_calf = right_leg
    thigh_cm = right_thigh * scale
    calf_cm = right_calf * scale
```

#### [measure/lambda_handler.py:281-289](measure/lambda_handler.py#L281-L289)

**반환 데이터 구조 개선:**
```python
return {
    "scale_cm_per_px": round(scale, 4),
    "upper_arm_cm": round(upper_arm_cm, 1) if upper_arm_cm else None,
    "forearm_cm": round(forearm_cm, 1) if forearm_cm else None,
    "thigh_cm": round(thigh_cm, 1) if thigh_cm else None,
    "calf_cm": round(calf_cm, 1) if calf_cm else None,
    "torso_cm": round(torso_cm, 1) if torso_cm else None,
    "output_image": overlay_path
}
```

#### [measure/lambda_handler.py:368-382](measure/lambda_handler.py#L368-L382)

**API 응답 형식 개선:**
```python
response_data = {
    'status': 'success',
    'data': {
        'scale_cm_per_px': result['scale_cm_per_px'],
        'upper_arm': result['upper_arm_cm'],
        'forearm': result['forearm_cm'],
        'thigh': result['thigh_cm'],
        'calf': result['calf_cm'],
        'torso': result['torso_cm'],
        'height': height_cm,  # 입력받은 키도 포함 (Recommend API 호출용)
        'output_image_url': result_url,
        'user_id': user_id
    }
}
```

### 3. 원본 measure.py와의 일치성

| 항목 | 원본 measure.py | Lambda 함수 (개선 후) | 일치 여부 |
|------|----------------|---------------------|----------|
| 상완 측정 | ✅ upper_arm_cm | ✅ upper_arm | ✅ |
| 전완 측정 | ✅ forearm_cm | ✅ forearm | ✅ |
| 허벅지 측정 | ✅ thigh_cm | ✅ thigh | ✅ |
| 종아리 측정 | ✅ calf_cm | ✅ calf | ✅ |
| 몸통 측정 | ✅ torso_cm | ✅ torso | ✅ |
| 키 정보 | ✅ height_cm (입력) | ✅ height | ✅ |
| 양쪽 평균 | ✅ | ✅ | ✅ |
| 한쪽만 측정 | ✅ | ✅ | ✅ |

## Measure → Recommend 워크플로우

### 1. Measure API 호출

```bash
POST /measure
{
  "image_url": "s3://hypermob-images/user123/photo.jpg",
  "height_cm": 175.0,
  "user_id": "user123"
}
```

### 2. Measure API 응답

```json
{
  "status": "success",
  "data": {
    "scale_cm_per_px": 0.8654,
    "upper_arm": 32.5,
    "forearm": 26.8,
    "thigh": 45.2,
    "calf": 38.9,
    "torso": 61.5,
    "height": 175.0,
    "output_image_url": "https://hypermob-results.s3.ap-northeast-2.amazonaws.com/results/user123/measurement_xxx.jpg",
    "user_id": "user123"
  }
}
```

### 3. Recommend API 호출 (Measure 응답 활용)

```bash
POST /recommend
{
  "height": 175.0,
  "upper_arm": 32.5,
  "forearm": 26.8,
  "thigh": 45.2,
  "calf": 38.9,
  "torso": 61.5,
  "user_id": "user123"
}
```

**중요:** Measure API 응답의 `data` 객체를 **그대로** Recommend API 요청으로 사용 가능! (output_image_url 제외)

### 4. Recommend API 응답

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
  "user_id": "user123"
}
```

## 재배포 방법

### PowerShell (권장)

```powershell
# Measure 함수만 재배포
.\rebuild_measure.ps1
```

### Git Bash

```bash
ACCOUNT_ID=$(aws sts get-caller-identity --query Account --output text)
AWS_REGION="ap-northeast-2"

# ECR 로그인
aws ecr get-login-password --region $AWS_REGION | \
    docker login --username AWS --password-stdin \
    $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com

# Measure 함수 리빌드
cd measure
docker build -t hypermob-measure .
docker tag hypermob-measure:latest \
    $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-measure:latest
docker push $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-measure:latest
cd ..

# Lambda 업데이트
aws lambda update-function-code \
    --function-name hypermob-measure \
    --image-uri $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-measure:latest \
    --region $AWS_REGION

aws lambda wait function-updated \
    --function-name hypermob-measure \
    --region $AWS_REGION
```

## 테스트

### 1. Measure API 단독 테스트

```powershell
$apiUrl = "https://cq66b70sd0.execute-api.ap-northeast-2.amazonaws.com/prod/measure"

$body = @{
    image_url = "s3://hypermob-images/test/full_body.jpg"
    height_cm = 175.0
    user_id = "test_user"
} | ConvertTo-Json

$response = Invoke-RestMethod -Uri $apiUrl 
    -Method Post 
    -Body $body 
    -ContentType "application/json"

Write-Host "Measure 결과:" -ForegroundColor Green
$response | ConvertTo-Json -Depth 10
```

**예상 응답:**
```json
{
  "status": "success",
  "data": {
    "scale_cm_per_px": 0.8654,
    "upper_arm": 32.5,
    "forearm": 26.8,
    "thigh": 45.2,
    "calf": 38.9,
    "torso": 61.5,
    "height": 175.0,
    "output_image_url": "https://...",
    "user_id": "test_user"
  }
}
```

### 2. Measure → Recommend 연계 테스트

```powershell
$measureUrl = "https://cq66b70sd0.execute-api.ap-northeast-2.amazonaws.com/prod/measure"
$recommendUrl = "https://cq66b70sd0.execute-api.ap-northeast-2.amazonaws.com/prod/recommend"

# Step 1: Measure API 호출
$measureBody = @{
    image_url = "s3://hypermob-images/test/full_body.jpg"
    height_cm = 175.0
    user_id = "test_user"
} | ConvertTo-Json

Write-Host "[1/2] Calling Measure API..." -ForegroundColor Yellow
$measureResponse = Invoke-RestMethod -Uri $measureUrl 
    -Method Post 
    -Body $measureBody 
    -ContentType "application/json"

Write-Host "Measure 결과:" -ForegroundColor Green
$measureResponse.data | ConvertTo-Json

# Step 2: Measure 결과를 Recommend API에 전달
$recommendBody = @{
    height = $measureResponse.data.height
    upper_arm = $measureResponse.data.upper_arm
    forearm = $measureResponse.data.forearm
    thigh = $measureResponse.data.thigh
    calf = $measureResponse.data.calf
    torso = $measureResponse.data.torso
    user_id = $measureResponse.data.user_id
} | ConvertTo-Json

Write-Host "`n[2/2] Calling Recommend API..." -ForegroundColor Yellow
$recommendResponse = Invoke-RestMethod -Uri $recommendUrl 
    -Method Post 
    -Body $recommendBody 
    -ContentType "application/json"

Write-Host "Recommend 결과:" -ForegroundColor Green
$recommendResponse.data | ConvertTo-Json

Write-Host "`n=========================================" -ForegroundColor Cyan
Write-Host " 연계 테스트 완료!" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
```

### 3. 전체 엔드포인트 테스트

```powershell
.\test_all_endpoints.ps1
```

## CloudWatch Logs 확인

```bash
# Measure 함수 로그
aws logs tail /aws/lambda/hypermob-measure --follow --region ap-northeast-2
```

**정상 로그:**
```
Processing image: /tmp/full_body.jpg
Image size: 1920x1080
Scale: 0.8654 cm/px
Measurement complete: {
  "status": "success",
  "data": {
    "upper_arm": 32.5,
    "forearm": 26.8,
    "thigh": 45.2,
    "calf": 38.9,
    "torso": 61.5,
    "height": 175.0
  }
}
```

## 변경사항 요약

| 변경 항목 | 이전 | 이후 | 효과 |
|----------|------|------|------|
| 팔 측정 | `arm_cm` (합계) | `upper_arm`, `forearm` (분리) | ✅ Recommend API 호환 |
| 다리 측정 | `leg_cm` (합계) | `thigh`, `calf` (분리) | ✅ Recommend API 호환 |
| 필드명 | `*_cm` | `upper_arm`, `forearm`, etc. | ✅ API 일관성 |
| 키 정보 | ❌ 없음 | `height` 포함 | ✅ Recommend API 요구사항 |
| 사용자 ID | ❌ 응답에 없음 | `user_id` 포함 | ✅ 추적성 향상 |
| 원본 로직 | ❌ 다름 | ✅ measure.py와 동일 | ✅ 정확도 보장 |

## 다음 단계

1. **Measure 함수 재배포**: `.\rebuild_measure.ps1` 실행
2. **Recommend 함수 재배포**: `.\rebuild_recommend.ps1` 실행 (S3 모델 로딩)
3. **전체 테스트**: `.\test_all_endpoints.ps1` 실행
4. **연계 테스트**: Measure → Recommend 워크플로우 검증

## 체크리스트

배포 전:
- [x] arm_length() 함수 수정 (튜플 반환)
- [x] leg_length() 함수 수정 (튜플 반환)
- [x] measure_body() 함수 수정 (상완/전완/허벅지/종아리 분리)
- [x] 반환 데이터 구조 수정 (upper_arm_cm, forearm_cm, thigh_cm, calf_cm)
- [x] API 응답 형식 수정 (upper_arm, forearm, thigh, calf, height)
- [x] 원본 measure.py 로직과 일치 확인
- [ ] Docker 이미지 리빌드
- [ ] Lambda 함수 업데이트
- [ ] 테스트 통과 (measure API 200 OK)
- [ ] Measure → Recommend 연계 테스트

배포 후:
- [ ] CloudWatch Logs 확인 (신체 치수 분리 측정 확인)
- [ ] Measure API 응답 필드 확인 (upper_arm, forearm, thigh, calf, torso, height)
- [ ] Recommend API 호출 성공 확인 (Measure 결과 활용)
- [ ] 측정 정확도 검증 (원본 measure.py와 비교)
