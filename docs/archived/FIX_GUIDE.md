# Lambda 함수 수정 및 재배포 가이드

## 발견된 문제

### 1. Recommend API 타임아웃
**원인**: 모델 파일(`car_fit_model.joblib`)이 Docker 이미지에 포함되지 않음

**해결**:
- ✅ `car_fit_model.joblib`을 `recommend/` 폴더로 복사
- ✅ `recommend/Dockerfile`에서 모델 파일 복사 활성화

### 2. Measure API - MediaPipe Read-only 에러
**원인**: MediaPipe가 Lambda의 read-only 파일시스템에 파일 쓰기 시도

**해결**:
- ✅ 환경 변수 설정으로 `/tmp` 사용하도록 변경
- ✅ `measure/lambda_handler.py`에 환경 변수 추가

## 수정된 파일

### 1. [measure/lambda_handler.py](measure/lambda_handler.py)
```python
# MediaPipe 환경 변수 설정 (read-only 파일시스템 문제 해결)
os.environ['MEDIAPIPE_DISABLE_GPU'] = '1'
os.environ['GLOG_logtostderr'] = '1'
os.environ['TMPDIR'] = '/tmp'
```

### 2. [recommend/Dockerfile](recommend/Dockerfile)
```dockerfile
# 사전 학습된 모델 복사
COPY car_fit_model.joblib ${LAMBDA_TASK_ROOT}/
```

### 3. [redeploy_functions.ps1](redeploy_functions.ps1)
Lambda 함수만 빠르게 재배포하는 스크립트

## 재배포 방법

### 방법 1: 자동 재배포 스크립트 (권장)

```powershell
# 모든 단계 자동 수행
.\redeploy_functions.ps1
```

이 스크립트는:
1. ECR 로그인
2. Docker 이미지 리빌드 (measure & recommend)
3. ECR에 푸시
4. Lambda 함수 업데이트
5. 자동 테스트 실행

**소요 시간**: 약 5-10분

### 방법 2: 수동 재배포

#### Step 1: 모델 파일 복사 확인
```powershell
# recommend 폴더에 모델 파일이 있는지 확인
Test-Path recommend/car_fit_model.joblib

# 없으면 복사
if (-not (Test-Path recommend/car_fit_model.joblib)) {
    Copy-Item car_fit_model.joblib recommend/
}
```

#### Step 2: Docker 이미지 리빌드
```powershell
$AccountId = aws sts get-caller-identity --query Account --output text
$AwsRegion = "ap-northeast-2"

# ECR 로그인
aws ecr get-login-password --region $AwsRegion | docker login --username AWS --password-stdin "$AccountId.dkr.ecr.$AwsRegion.amazonaws.com"

# measure 함수 리빌드
cd measure
docker build -t hypermob-measure .
docker tag hypermob-measure:latest "$AccountId.dkr.ecr.$AwsRegion.amazonaws.com/hypermob-measure:latest"
docker push "$AccountId.dkr.ecr.$AwsRegion.amazonaws.com/hypermob-measure:latest"
cd ..

# recommend 함수 리빌드
cd recommend
docker build -t hypermob-recommend .
docker tag hypermob-recommend:latest "$AccountId.dkr.ecr.$AwsRegion.amazonaws.com/hypermob-recommend:latest"
docker push "$AccountId.dkr.ecr.$AwsRegion.amazonaws.com/hypermob-recommend:latest"
cd ..
```

#### Step 3: Lambda 함수 업데이트
```powershell
# measure 함수 업데이트
aws lambda update-function-code 
    --function-name hypermob-measure 
    --image-uri "$AccountId.dkr.ecr.$AwsRegion.amazonaws.com/hypermob-measure:latest" 
    --region $AwsRegion

# 업데이트 완료 대기
aws lambda wait function-updated --function-name hypermob-measure --region $AwsRegion

# recommend 함수 업데이트
aws lambda update-function-code 
    --function-name hypermob-recommend 
    --image-uri "$AccountId.dkr.ecr.$AwsRegion.amazonaws.com/hypermob-recommend:latest" 
    --region $AwsRegion

# 업데이트 완료 대기
aws lambda wait function-updated --function-name hypermob-recommend --region $AwsRegion
```

#### Step 4: 테스트
```powershell
.\test_all_endpoints.ps1
```

## Git Bash로 재배포

```bash
# ECR 로그인
ACCOUNT_ID=$(aws sts get-caller-identity --query Account --output text)
AWS_REGION="ap-northeast-2"

aws ecr get-login-password --region $AWS_REGION | \
    docker login --username AWS --password-stdin \
    $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com

# measure 이미지 빌드 및 푸시
cd measure
docker build -t hypermob-measure .
docker tag hypermob-measure:latest $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-measure:latest
docker push $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-measure:latest
cd ..

# recommend 이미지 빌드 및 푸시
cd recommend
docker build -t hypermob-recommend .
docker tag hypermob-recommend:latest $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-recommend:latest
docker push $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-recommend:latest
cd ..

# Lambda 함수 업데이트
aws lambda update-function-code \
    --function-name hypermob-measure \
    --image-uri $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-measure:latest \
    --region $AWS_REGION

aws lambda wait function-updated --function-name hypermob-measure --region $AWS_REGION

aws lambda update-function-code \
    --function-name hypermob-recommend \
    --image-uri $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-recommend:latest \
    --region $AWS_REGION

aws lambda wait function-updated --function-name hypermob-recommend --region $AWS_REGION

# 테스트
./test_all_endpoints.sh
```

## 예상 결과

재배포 후 테스트 실행 시:

```
=========================================
 Test 2: Recommend API - Valid Request
=========================================

[TEST] Testing POST /recommend with valid data
[INFO] Response received in 1500ms
{
  "status": "success",
  "data": {
    "seat_position": 46,
    "seat_angle": 15,
    ...
  }
}
[PASS] Status is 'success'
[PASS] All required fields present
[PASS] Recommend API test passed
[PASS] Response time < 5s

=========================================
 Test 5: Measure API
=========================================

[TEST] Testing POST /measure
[INFO] Response received in 25s
{
  "status": "success",
  "data": {
    "arm_cm": 65.3,
    "leg_cm": 92.1,
    "torso_cm": 61.5,
    ...
  }
}
[PASS] Measure API test passed
```

## 트러블슈팅

### Docker 빌드 실패
```powershell
# Docker 데몬 확인
docker ps

# Docker Desktop 재시작
```

### ECR 푸시 실패
```powershell
# ECR 재로그인
aws ecr get-login-password --region ap-northeast-2 | 
    docker login --username AWS --password-stdin 
    $AccountId.dkr.ecr.ap-northeast-2.amazonaws.com
```

### Lambda 업데이트 실패
```bash
# Lambda 함수 상태 확인
aws lambda get-function --function-name hypermob-measure --region ap-northeast-2

# 로그 확인
aws logs tail /aws/lambda/hypermob-measure --follow --region ap-northeast-2
```

## 향후 개선 사항

### 1. Measure → Recommend 통합 워크플로우

현재는 두 API가 독립적입니다. 통합 엔드포인트를 만들 수 있습니다:

```
POST /analyze
{
  "image_url": "s3://...",
  "height_cm": 175.0
}

→ 자동으로 measure + recommend 수행
→ 차량 설정 바로 반환
```

### 2. S3에서 모델 로드

모델 파일을 S3에 저장하고 Lambda에서 동적 로드:

```python
# Lambda 시작 시 S3에서 모델 다운로드
s3_client.download_file('hypermob-models', 'car_fit_model.joblib', '/tmp/model.joblib')
model = joblib.load('/tmp/model.joblib')
```

장점:
- Docker 이미지 크기 감소
- 모델 업데이트 시 Lambda 재배포 불필요

### 3. Step Functions 워크플로우

```
1. Measure Lambda 실행
   ↓
2. 결과를 DynamoDB에 저장
   ↓
3. Recommend Lambda 실행
   ↓
4. 최종 결과 반환
```

## 참고 자료

- [TEST_SCENARIOS.md](TEST_SCENARIOS.md) - 상세 테스트 시나리오
- [DEPLOYMENT.md](DEPLOYMENT.md) - 전체 배포 가이드
- [README_LAMBDA.md](README_LAMBDA.md) - Lambda 함수 문서
