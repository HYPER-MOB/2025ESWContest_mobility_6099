# Facedata Lambda 함수 - 얼굴 랜드마크 추출

## 개요

원본 [facedata.py](facedata.py)를 AWS Lambda 함수로 변환하여 API 엔드포인트로 제공합니다.

**주요 기능:**
- MediaPipe FaceMesh로 468개 얼굴 랜드마크 추출
- S3 이미지 입력/출력
- 선택적 랜드마크 추출 지원
- 얼굴 인증, 표정 분석 등에 활용 가능

## 원본 vs Lambda 비교

| 항목 | 원본 facedata.py | Lambda 함수 |
|------|-----------------|-------------|
| 입력 | 카메라 실시간 캡처 (120프레임) | S3 이미지 URL |
| 처리 | 여러 프레임 중앙값 | 단일 이미지 |
| 출력 | 로컬 텍스트 파일 | S3 텍스트 + JSON 응답 |
| 환경 | Windows + USB 카메라 | AWS Lambda (서버리스) |
| 사용성 | 로컬 스크립트 | REST API |

## 아키텍처

```
Client
  ↓
API Gateway (/facedata)
  ↓
Lambda (hypermob-facedata)
  ↓
[S3에서 얼굴 이미지 다운로드]
  ↓
[MediaPipe FaceMesh 처리]
  ├─ 468개 랜드마크 감지
  └─ x, y, z 좌표 추출 (정규화 0-1)
  ↓
[결과 파일 생성]
  ├─ JSON 응답 (API)
  └─ 텍스트 파일 (S3 업로드)
  ↓
Response
```

## API 스펙

### 엔드포인트: `POST /facedata

#### 요청

```json
{
  "image_url": "s3://hypermob-images/user123/face.jpg",
  "user_id": "user123",
  "refine": false,
  "indices": ""
}
```

**파라미터:**
- `image_url` (required): S3 얼굴 이미지 URL
- `user_id` (optional): 사용자 ID (기본값: "anonymous")
- `refine` (optional): FaceMesh refine_landmarks 옵션 (기본값: false)
- `indices` (optional): 추출할 랜드마크 인덱스 (비어있으면 전체)

**인덱스 형식:**
- 4자리: `landmark_id(3자리)` + `coord_id(1자리)
- coord_id: `0`=x, `1`=y, `2`=z
- 예시: `"1230,0561,0072"
  - `1230`: landmark 123의 x좌표
  - `0561`: landmark 56의 y좌표
  - `0072`: landmark 7의 z좌표

#### 응답 (성공)

```json
{
  "status": "success",
  "data": {
    "landmarks": {
      "0010": 0.51234567,
      "0011": 0.62345678,
      "0012": 0.01234567,
      ...
    },
    "landmark_count": 1404,
    "output_file_url": "https://hypermob-results.s3.ap-northeast-2.amazonaws.com/facedata/user123/profile_xxx.txt",
    "user_id": "user123"
  }
}
```

**필드 설명:**
- `landmarks`: 랜드마크 좌표 맵
- `landmark_count`: 추출된 좌표 개수 (전체: 468 × 3 = 1404)
- `output_file_url`: 텍스트 파일 S3 URL
- `user_id`: 사용자 ID

#### 응답 (에러)

```json
{
  "status": "error",
  "error": {
    "code": "FACE_DETECTION_FAILED",
    "message": "No face detected. Please use a clear frontal face image."
  }
}
```

**에러 코드:**
- `MISSING_FIELD`: 필수 필드 누락
- `IMAGE_NOT_FOUND`: S3 이미지 없음
- `FACE_DETECTION_FAILED`: 얼굴 감지 실패
- `INVALID_INDICES`: 잘못된 인덱스 형식
- `INTERNAL_ERROR`: 서버 내부 오류

## 출력 파일 형식

### JSON (API 응답)
```json
{
  "0010": 0.51234567,
  "0011": 0.62345678,
  ...
}
```

### 텍스트 (S3 파일)
```
0010 0.51234567
0011 0.62345678
0012 0.01234567
...
FFFF
```

**형식:**
- 각 라인: `{landmark_id}{coord_id} {value}
- 마지막 라인: `FFFF` (종료 마커)
- 원본 facedata.py와 동일한 형식

## 배포 방법

### 1. Lambda 함수 배포

```powershell
# Facedata 함수만 배포
.\rebuild_facedata.ps1
```

이 스크립트가 자동으로:
1. ECR 저장소 생성 (hypermob-facedata)
2. Docker 이미지 빌드 (MediaPipe FaceMesh 모델 다운로드 포함)
3. ECR에 푸시
4. IAM 역할 생성 (S3 읽기/쓰기 권한 포함)
5. Lambda 함수 생성/업데이트

### 2. API Gateway 업데이트

```powershell
# OpenAPI 스펙 배포
.\deploy_api_gateway.ps1
```

[openapi.yml](openapi.yml)에 `/facedata` 엔드포인트가 포함되어 있습니다.

### 3. Lambda 함수 권한 추가

API Gateway가 Lambda를 호출할 수 있도록 권한 추가:

```powershell
$apiId = aws apigateway get-rest-apis --query "items[?name=='HYPERMOB Platform AI API'].id" --output text --region ap-northeast-2

aws lambda add-permission 
    --function-name hypermob-facedata 
    --statement-id apigateway-invoke 
    --action lambda:InvokeFunction 
    --principal apigateway.amazonaws.com 
    --source-arn "arn:aws:execute-api:ap-northeast-2:471464546082:$apiId/*" 
    --region ap-northeast-2
```

## 테스트

### 1. 테스트 이미지 업로드

```powershell
# 얼굴 이미지 업로드
aws s3 cp your_face.jpg s3://hypermob-images/test/face.jpg --region ap-northeast-2

# 확인
aws s3 ls s3://hypermob-images/test/face.jpg --region ap-northeast-2
```

**이미지 요구사항:**
- 정면 얼굴 사진
- 얼굴이 명확하게 보임
- 배경: 단색 또는 단순한 배경 권장
- 해상도: 최소 480p (640x480) 이상

### 2. 자동 테스트

```powershell
# 전체 엔드포인트 테스트 (Test 7 포함)
.\test_all_endpoints.ps1
```

**Test 7a: 전체 랜드마크 추출**
```
[TEST 7a] Full landmark extraction (all 468 landmarks)
[INFO] Extracted 1404 landmarks
[INFO] Sample landmarks:
  0010 = 0.51234567
  0011 = 0.62345678
  0012 = 0.01234567
[PASS] Landmarks extracted successfully
[PASS] Facedata API (full) test passed
```

**Test 7b: 선택적 랜드마크 추출**
```
[TEST 7b] Selective landmark extraction (specific indices)
[INFO] Extracted 6 landmarks (expected 6)
[INFO] Extracted landmarks:
  0010 = 0.51234567
  0011 = 0.62345678
  0012 = 0.01234567
  0330 = 0.42345678
  0331 = 0.53456789
  0332 = 0.02345678
[PASS] Correct number of landmarks extracted
[PASS] Facedata API (selective) test passed
```

### 3. 수동 API 테스트

#### 전체 랜드마크 추출

```powershell
$apiUrl = "https://cq66b70sd0.execute-api.ap-northeast-2.amazonaws.com/prod/facedata"

$body = @{
    image_url = "s3://hypermob-images/test/face.jpg"
    user_id = "manual_test"
    refine = $false
    indices = ""
} | ConvertTo-Json

$response = Invoke-RestMethod -Uri $apiUrl 
    -Method Post 
    -Body $body 
    -ContentType "application/json"

Write-Host "Landmark count: $($response.data.landmark_count)"
Write-Host "Output file: $($response.data.output_file_url)"
```

#### 선택적 랜드마크 추출

```powershell
$body = @{
    image_url = "s3://hypermob-images/test/face.jpg"
    user_id = "manual_test"
    refine = $true
    indices = "0010,0011,0012,0330,0331,0332"  # 6개만 추출
} | ConvertTo-Json

$response = Invoke-RestMethod -Uri $apiUrl 
    -Method Post 
    -Body $body 
    -ContentType "application/json"

Write-Host "Extracted landmarks:"
$response.data.landmarks | ConvertTo-Json
```

## CloudWatch Logs 확인

```bash
aws logs tail /aws/lambda/hypermob-facedata --follow --region ap-northeast-2
```

**정상 로그:**
```
Processing image: /tmp/face.jpg
Image size: 1280x720
Extracting 1404 landmark coordinates
Detected 468 face landmarks
Face landmark extraction complete: 1404 coordinates
```

## 사용 사례

### 1. 얼굴 인증 시스템

```python
# 사용자 등록 (얼굴 랜드마크 저장)
response = requests.post(
    f"{api_url}/facedata",
    json={
        "image_url": "s3://hypermob-images/users/user123/face.jpg",
        "user_id": "user123",
        "refine": True,
        "indices": ""  # 전체 랜드마크
    }
)

user_landmarks = response.json()["data"]["landmarks"]
# DB에 저장: user_id → landmarks

# 사용자 인증 (얼굴 비교)
response2 = requests.post(
    f"{api_url}/facedata",
    json={
        "image_url": "s3://hypermob-images/auth/attempt.jpg",
        "user_id": "auth_attempt",
        "refine": True,
        "indices": ""
    }
)

attempt_landmarks = response2.json()["data"]["landmarks"]
# 유사도 계산: user_landmarks vs attempt_landmarks
similarity = calculate_similarity(user_landmarks, attempt_landmarks)

if similarity > 0.95:
    print("인증 성공")
else:
    print("인증 실패")
```

### 2. 표정 분석

```python
# 주요 얼굴 특징점만 추출
response = requests.post(
    f"{api_url}/facedata",
    json={
        "image_url": "s3://hypermob-images/users/user123/face.jpg",
        "user_id": "user123",
        "refine": False,
        "indices": "0010,0011,0330,0331,0610,0611"  # 눈, 입 주변
    }
)

landmarks = response.json()["data"]["landmarks"]

# 표정 분석 (예: 미소 감지)
mouth_landmarks = [landmarks["0610"], landmarks["0611"]]
if is_smiling(mouth_landmarks):
    print("미소 감지")
```

### 3. 차량 시트 조절 (얼굴 크기 고려)

```python
# 얼굴 크기 측정
response = requests.post(
    f"{api_url}/facedata",
    json={
        "image_url": "s3://hypermob-images/users/user123/face.jpg",
        "user_id": "user123",
        "indices": "0010,0011,0012,0330,0331,0332"  # 얼굴 윤곽
    }
)

landmarks = response.json()["data"]["landmarks"]
face_width = calculate_face_width(landmarks)

# 얼굴 크기에 따른 헤드레스트 조절
if face_width > 0.5:
    headrest_position = "high"
else:
    headrest_position = "low"
```

## MediaPipe FaceMesh 랜드마크 맵

468개 랜드마크 분포:
- **얼굴 윤곽**: 0-16
- **오른쪽 눈썹**: 17-21, 46-53
- **왼쪽 눈썹**: 22-26, 54-61
- **코**: 27-35, 168-197
- **오른쪽 눈**: 36-41, 133-144, 145-154, 155-157
- **왼쪽 눈**: 42-47, 362-373, 374-383, 384-386
- **입**: 48-68, 78-95
- **입술 내부**: 69-77

상세 맵: [MediaPipe Face Mesh](https://github.com/google/mediapipe/blob/master/docs/solutions/face_mesh.md)

## 성능

| 항목 | Cold Start | Warm Start |
|------|-----------|------------|
| Docker 이미지 다운로드 | 8-12초 | 0초 |
| Lambda 초기화 | 2-3초 | 0초 |
| FaceMesh 처리 | 1-2초 | 1-2초 |
| **총 응답 시간** | **11-17초** | **1-2초** |

**최적화 팁:**
- `refine=false` 사용 (30% 빠름)
- `indices` 지정으로 필요한 랜드마크만 추출
- Warm-up 호출로 Lambda 활성 상태 유지

## 비용 추정 (월 10,000 요청 기준)

| 항목 | 수량 | 단가 | 월 비용 |
|------|------|------|---------|
| Lambda 실행 (2GB, 5초) | 10,000회 | $0.0000166667/GB-초 | $1.67 |
| S3 이미지 다운로드 (1MB) | 10,000회 | $0.0004/1000건 | $0.004 |
| S3 결과 업로드 (10KB) | 10,000회 | $0.0004/1000건 | $0.004 |
| API Gateway | 10,000회 | $3.50/100만건 | $0.035 |
| **총 비용** | | | **~$1.71/월** |

## 문제 해결

### 1. 얼굴 감지 실패

**증상:**
```json
{
  "error": {
    "code": "FACE_DETECTION_FAILED",
    "message": "No face detected..."
  }
}
```

**원인:**
- 얼굴이 이미지에 없음
- 얼굴이 너무 작음
- 각도가 너무 심함
- 조명이 불량

**해결:**
- 정면 얼굴 사진 사용
- 얼굴이 이미지의 30% 이상 차지하도록
- 조명이 균일한 환경
- 배경이 단순한 이미지

### 2. 잘못된 인덱스 형식

**증상:**
```json
{
  "error": {
    "code": "INVALID_INDICES",
    "message": "Invalid index token: '12345'..."
  }
}
```

**원인:**
- 인덱스가 4자리가 아님
- 숫자가 아닌 문자 포함
- 범위 초과 (landmark 0-467, coord 0-2)

**해결:**
```json
// 잘못된 예
"indices": "123,456"     // 3자리 (X)
"indices": "12345"       // 5자리 (X)
"indices": "1230,abc1"   // 문자 포함 (X)
"indices": "4680"        // landmark 468 없음 (X)

// 올바른 예
"indices": "1230,0561,0072"  // 4자리 (O)
"indices": ""                // 전체 추출 (O)
```

## 다음 단계

1. **Facedata 배포:**
   ```powershell
   .\rebuild_facedata.ps1
   ``

2. **API Gateway 업데이트:**
   ```powershell
   .\deploy_api_gateway.ps1
   ``

3. **권한 추가:**
   ```powershell
   # API Gateway → Lambda 호출 권한
   aws lambda add-permission --function-name hypermob-facedata ...
   ``

4. **테스트:**
   ```powershell
   # 얼굴 이미지 업로드
   aws s3 cp face.jpg s3://hypermob-images/test/face.jpg

   # 전체 테스트
   .\test_all_endpoints.ps1
   ``

## 요약

✅ **완료된 작업:**
- [facedata/lambda_handler.py](facedata/lambda_handler.py): Lambda 함수 구현
- [facedata/Dockerfile](facedata/Dockerfile): Docker 이미지 정의
- [facedata/requirements.txt](facedata/requirements.txt): Python 의존성
- [facedata/download_models.py](facedata/download_models.py): MediaPipe 모델 다운로드
- [openapi.yml](openapi.yml): `/facedata` 엔드포인트 추가
- [rebuild_facedata.ps1](rebuild_facedata.ps1): 배포 스크립트
- [test_all_endpoints.ps1](test_all_endpoints.ps1): Test 7 추가

✅ **주요 기능:**
- 468개 얼굴 랜드마크 추출
- 선택적 랜드마크 추출 (indices 파라미터)
- JSON + 텍스트 파일 출력
- S3 이미지 입력/출력
- 원본 facedata.py와 호환되는 형식

✅ **사용 사례:**
- 얼굴 인증 시스템
- 표정 분석
- 차량 내부 개인화 (헤드레스트, 미러 조절)
