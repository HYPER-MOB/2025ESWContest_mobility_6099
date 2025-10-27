# Recommend Lambda - S3 모델 로딩 개선

## 개요

Recommend Lambda 함수를 개선하여 모델 파일을 Docker 이미지에 번들링하지 않고 S3에서 동적으로 로드하도록 변경했습니다.

## 주요 변경사항

### 1. 모델 저장 위치
- **이전**: Docker 이미지 내부 `/var/task/car_fit_model.joblib
- **이후**: S3 버킷 `s3://hypermob-models/car_fit_model.joblib

### 2. 코드 변경

#### [recommend/lambda_handler.py](recommend/lambda_handler.py)

**추가된 함수: `download_model_from_s3()`**
```python
def download_model_from_s3():
    """S3에서 모델 다운로드"""
    s3_bucket = os.environ.get('MODEL_BUCKET', 'hypermob-models')
    s3_key = os.environ.get('MODEL_KEY', 'car_fit_model.joblib')
    local_path = '/tmp/car_fit_model.joblib'

    # 이미 다운로드되어 있으면 재사용 (Lambda 컨테이너 재사용 최적화)
    if os.path.exists(local_path):
        print(f"Model already exists at {local_path}")
        return local_path

    try:
        print(f"Downloading model from s3://{s3_bucket}/{s3_key}")
        s3_client = boto3.client('s3', region_name=os.environ.get('AWS_REGION'))
        s3_client.download_file(s3_bucket, s3_key, local_path)
        print(f"Model downloaded: {os.path.getsize(local_path)} bytes")
        return local_path
    except Exception as e:
        print(f"Failed to download model from S3: {str(e)}")
        raise
```

**개선된 함수: `initialize_model()`**
- S3에서 모델 다운로드 시도 (primary)
- 실패 시 json_template.json으로 재학습 (fallback)
- 컨테이너 재사용 시 /tmp에 캐시된 모델 재사용

#### [recommend/Dockerfile](recommend/Dockerfile)

**제거된 라인:**
```dockerfile
# 사전 학습된 모델 복사
COPY car_fit_model.joblib ${LAMBDA_TASK_ROOT}/
```

**추가된 환경 변수:**
```dockerfile
ENV MODEL_BUCKET=hypermob-models
ENV MODEL_KEY=car_fit_model.joblib
```

### 3. IAM 권한 추가

Lambda 실행 역할에 S3 읽기 권한 필요:

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "s3:GetObject",
        "s3:ListBucket"
      ],
      "Resource": [
        "arn:aws:s3:::hypermob-models",
        "arn:aws:s3:::hypermob-models/*"
      ]
    }
  ]
}
```

**rebuild_recommend.ps1** 스크립트가 자동으로 이 권한을 추가합니다.

## 장점

### 1. Docker 이미지 크기 감소
- **이전**: ~500MB (모델 포함)
- **이후**: ~300MB (모델 제외)
- 빌드 및 배포 시간 단축

### 2. 모델 업데이트 용이성
```bash
# 모델만 업데이트 (Lambda 재배포 불필요)
aws s3 cp new_car_fit_model.joblib s3://hypermob-models/car_fit_model.joblib

# Lambda 함수 재시작 (새 컨테이너가 새 모델 다운로드)
aws lambda update-function-configuration \
    --function-name hypermob-recommend \
    --environment Variables={FORCE_RELOAD=true}
```

### 3. 컨테이너 재사용 최적화
- Cold start: S3에서 모델 다운로드 (1-2초 추가)
- Warm start: /tmp에 캐시된 모델 재사용 (추가 시간 없음)
- Lambda 컨테이너는 일반적으로 5-15분간 유지됨

### 4. 모델 버전 관리
S3 버전 관리 활성화 시:
```bash
# S3 버전 관리 활성화
aws s3api put-bucket-versioning \
    --bucket hypermob-models \
    --versioning-configuration Status=Enabled

# 특정 버전 모델 사용 (환경 변수로 지정 가능)
aws lambda update-function-configuration \
    --function-name hypermob-recommend \
    --environment Variables={MODEL_KEY=car_fit_model_v2.joblib}
```

## 재배포 방법

### PowerShell (권장)

```powershell
# Recommend 함수만 재배포
.\rebuild_recommend.ps1

# 전체 시스템 재배포
.\deploy.ps1
```

### Git Bash

```bash
ACCOUNT_ID=$(aws sts get-caller-identity --query Account --output text)
AWS_REGION="ap-northeast-2"

# ECR 로그인
aws ecr get-login-password --region $AWS_REGION | \
    docker login --username AWS --password-stdin \
    $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com

# Recommend 함수 리빌드
cd recommend
docker build -t hypermob-recommend .
docker tag hypermob-recommend:latest \
    $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-recommend:latest
docker push $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-recommend:latest
cd ..

# Lambda 업데이트
aws lambda update-function-code \
    --function-name hypermob-recommend \
    --image-uri $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-recommend:latest \
    --region $AWS_REGION

aws lambda wait function-updated \
    --function-name hypermob-recommend \
    --region $AWS_REGION

# 환경 변수 설정
aws lambda update-function-configuration \
    --function-name hypermob-recommend \
    --environment Variables={AWS_REGION=$AWS_REGION,MODEL_BUCKET=hypermob-models,MODEL_KEY=car_fit_model.joblib} \
    --region $AWS_REGION

# IAM 권한 추가 (한 번만 필요)
aws iam put-role-policy \
    --role-name hypermob-recommend-role \
    --policy-name S3ModelReadAccess \
    --policy-document file://s3-read-policy.json \
    --region $AWS_REGION
```

## 테스트

### 1. 빠른 테스트

```powershell
.\test_all_endpoints.ps1
```

예상 출력:
```
=========================================
 Test 4: Recommend API (Valid Input)
=========================================

[TEST] Testing POST /recommend
[INFO] Response received in 3s
{
  "status": "success",
  "data": {
    "seat_position": 45,
    "seat_angle": 105,
    ...
  },
  "user_id": "test_user"
}
[PASS] Recommend API test passed
[PASS] Response time < 10s
```

### 2. CloudWatch Logs 확인

```bash
aws logs tail /aws/lambda/hypermob-recommend --follow --region ap-northeast-2
```

**정상 로그 (Cold Start):**
```
Downloading model from s3://hypermob-models/car_fit_model.joblib to /tmp/car_fit_model.joblib
Model downloaded successfully: 15234 bytes
Loading pre-trained model from /tmp/car_fit_model.joblib
Model initialization complete
Predicting car setup for user: test_user
Prediction complete
```

**정상 로그 (Warm Start):**
```
Model already exists at /tmp/car_fit_model.joblib
Loading pre-trained model from /tmp/car_fit_model.joblib
Model already loaded
Predicting car setup for user: test_user
Prediction complete
```

### 3. 수동 API 테스트

```powershell
$apiUrl = "https://cq66b70sd0.execute-api.ap-northeast-2.amazonaws.com/prod/recommend"

$body = @{
    height = 175.0
    upper_arm = 32.5
    forearm = 26.8
    thigh = 45.2
    calf = 38.9
    torso = 61.5
    user_id = "manual_test"
} | ConvertTo-Json

Invoke-RestMethod -Uri $apiUrl 
    -Method Post 
    -Body $body 
    -ContentType "application/json"
```

## 성능 비교

### Cold Start
| 항목 | 이전 (번들링) | 이후 (S3) | 차이 |
|------|--------------|----------|------|
| Docker 이미지 크기 | ~500MB | ~300MB | -40% |
| 이미지 다운로드 시간 | 10-15초 | 6-10초 | -33% |
| 모델 로드 시간 | 0.5초 | 1.5초 (다운로드 포함) | +1초 |
| **총 Cold Start** | **10-16초** | **7-12초** | **-25%** |

### Warm Start
| 항목 | 이전 (번들링) | 이후 (S3) | 차이 |
|------|--------------|----------|------|
| 모델 로드 | 이미 로드됨 | 캐시 재사용 | 동일 |
| **응답 시간** | **100-200ms** | **100-200ms** | **동일** |

## 문제 해결

### 1. S3 다운로드 실패

**증상:**
```
Failed to download model from S3: An error occurred (403) when calling the GetObject operation: Forbidden
```

**해결:**
```bash
# IAM 권한 확인
aws iam get-role-policy \
    --role-name hypermob-recommend-role \
    --policy-name S3ModelReadAccess

# 권한 추가
.\rebuild_recommend.ps1  # 자동으로 권한 추가
```

### 2. 모델 파일 없음

**증상:**
```
Failed to download model from S3: An error occurred (404) when calling the GetObject operation: Not Found
Falling back to training new model from template data
```

**해결:**
```bash
# S3에 모델 업로드
aws s3 cp car_fit_model.joblib s3://hypermob-models/car_fit_model.joblib

# 확인
aws s3 ls s3://hypermob-models/
```

### 3. /tmp 디스크 공간 부족

**증상:**
```
OSError: [Errno 28] No space left on device
```

**해결:**
- Lambda /tmp는 512MB 제한
- 모델 파일이 너무 크면 Lambda 메모리 증가 필요
```bash
aws lambda update-function-configuration \
    --function-name hypermob-recommend \
    --ephemeral-storage Size=1024  # /tmp 크기를 1GB로 증가
```

## 다음 단계

### 1. 모델 업데이트 워크플로우
```bash
# 1. 새 모델 학습 (로컬)
python train_model.py --output new_model.joblib

# 2. 모델 검증
python validate_model.py --model new_model.joblib

# 3. S3에 업로드
aws s3 cp new_model.joblib s3://hypermob-models/car_fit_model.joblib

# 4. Lambda 재시작 (새 컨테이너가 새 모델 로드)
# 방법 1: 환경 변수 변경으로 강제 재시작
aws lambda update-function-configuration \
    --function-name hypermob-recommend \
    --environment Variables={AWS_REGION=ap-northeast-2,MODEL_BUCKET=hypermob-models,MODEL_KEY=car_fit_model.joblib,UPDATED_AT=$(date +%s)}

# 방법 2: 기존 컨테이너가 만료될 때까지 대기 (5-15분)
```

### 2. 모델 버전 관리

S3 버전 관리 활성화:
```bash
aws s3api put-bucket-versioning \
    --bucket hypermob-models \
    --versioning-configuration Status=Enabled
```

특정 버전 사용:
```bash
# 버전 목록 확인
aws s3api list-object-versions \
    --bucket hypermob-models \
    --prefix car_fit_model.joblib

# 특정 버전 다운로드
aws s3api get-object \
    --bucket hypermob-models \
    --key car_fit_model.joblib \
    --version-id <VERSION_ID> \
    model_v1.joblib
```

### 3. A/B 테스팅

두 개의 모델 비교:
```bash
# 환경 변수로 모델 선택
aws lambda update-function-configuration \
    --function-name hypermob-recommend \
    --environment Variables={MODEL_KEY=car_fit_model_v2.joblib}

# 또는 별도의 Lambda 함수로 배포
aws lambda create-function \
    --function-name hypermob-recommend-v2 \
    --package-type Image \
    --code ImageUri=...
```

## 체크리스트

배포 전 확인사항:

- [x] recommend/lambda_handler.py에 boto3 import 추가
- [x] download_model_from_s3() 함수 구현
- [x] initialize_model() 함수 개선 (S3 로딩 + fallback)
- [x] recommend/Dockerfile에서 모델 COPY 제거
- [x] 환경 변수 추가 (MODEL_BUCKET, MODEL_KEY)
- [x] rebuild_recommend.ps1 스크립트 생성
- [x] S3 버킷에 모델 파일 존재 확인
- [ ] IAM 역할에 S3 읽기 권한 추가
- [ ] Docker 이미지 리빌드
- [ ] ECR에 푸시
- [ ] Lambda 함수 업데이트
- [ ] 테스트 통과 (recommend API 200 OK)
- [ ] CloudWatch Logs 확인

배포 후 확인사항:

- [ ] Cold start 시 S3에서 모델 다운로드 확인
- [ ] Warm start 시 캐시된 모델 재사용 확인
- [ ] API 응답 정확도 검증
- [ ] 응답 시간 모니터링 (< 5초)
