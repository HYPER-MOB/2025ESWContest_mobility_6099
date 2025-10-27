# HYPERMOB Platform AI - Lambda 함수

AWS Lambda로 배포되는 신체 측정 및 차량 설정 추천 AI 서비스입니다.

## 프로젝트 구조

```
Platform-AI/
├── measure/                      # 신체 측정 Lambda 함수
│   ├── lambda_handler.py        # Lambda 핸들러
│   ├── Dockerfile               # Docker 이미지 빌드 파일
│   └── requirements.txt         # Python 의존성
│
├── recommend/                    # 차량 설정 추천 Lambda 함수
│   ├── lambda_handler.py        # Lambda 핸들러
│   ├── Dockerfile               # Docker 이미지 빌드 파일
│   ├── requirements.txt         # Python 의존성
│   └── json_template.json       # 훈련 데이터
│
├── openapi.yml                  # API Gateway OpenAPI 스펙
├── DEPLOYMENT.md                # 상세 배포 가이드
├── deploy.sh                    # 자동 배포 스크립트
├── test_api.sh                  # API 테스트 스크립트
│
├── measure.py                   # 원본 측정 스크립트
├── recommend.py                 # 원본 추천 스크립트
└── json_template.json           # 훈련 데이터 템플릿
```

## 빠른 시작

### 1. 사전 준비

```bash
# AWS CLI 설정
aws configure

# Docker 설치 확인
docker --version
```

### 2. 배포

```bash
# 배포 스크립트 실행 권한 부여
chmod +x deploy.sh

# 전체 배포 실행
./deploy.sh
```

배포 스크립트는 다음을 자동으로 수행합니다:
- S3 버킷 생성 (hypermob-images, hypermob-models, hypermob-results)
- IAM 역할 생성
- ECR 리포지토리 생성
- Docker 이미지 빌드 및 푸시
- Lambda 함수 생성/업데이트
- API Gateway 설정
- 권한 부여 및 API 배포

### 3. 테스트

```bash
# 테스트 스크립트 실행 권한 부여
chmod +x test_api.sh

# API 테스트 실행
./test_api.sh
```

## Lambda 함수 설명

### measure 함수

**기능**: 전신 이미지에서 신체 치수 측정

**입력**:
```json
{
  "image_url": "s3://hypermob-images/users/user123/full_body.jpg",
  "height_cm": 175.0,
  "user_id": "user123"
}
```

**출력**:
```json
{
  "status": "success",
  "data": {
    "scale_cm_per_px": 0.2145,
    "arm_cm": 65.3,
    "leg_cm": 92.1,
    "torso_cm": 61.5,
    "output_image_url": "https://hypermob-results.s3.ap-northeast-2.amazonaws.com/results/user123/measurement_xxx.jpg"
  }
}
```

**리소스**:
- 메모리: 3008 MB
- 타임아웃: 300초 (5분)
- 런타임: Python 3.9 (Docker)

**의존성**:
- opencv-python-headless
- mediapipe
- numpy
- boto3

### recommend 함수

**기능**: 신체 치수 기반 차량 설정 추천

**입력**:
```json
{
  "height": 175.0,
  "upper_arm": 31.0,
  "forearm": 26.0,
  "thigh": 51.0,
  "calf": 36.0,
  "torso": 61.0,
  "user_id": "user123"
}
```

**출력**:
```json
{
  "status": "success",
  "data": {
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
  "user_id": "user123"
}
```

**리소스**:
- 메모리: 1024 MB
- 타임아웃: 60초
- 런타임: Python 3.9 (Docker)

**의존성**:
- numpy
- pandas
- scikit-learn
- joblib
- boto3

## API 엔드포인트

배포 후 다음과 같은 형식의 URL이 생성됩니다:

```
https://{api-id}.execute-api.ap-northeast-2.amazonaws.com/prod
```

### 사용 가능한 엔드포인트

- `GET /health` - 헬스 체크
- `POST /measure` - 신체 측정
- `POST /recommend` - 차량 설정 추천

## 로컬 테스트

### measure 함수 로컬 테스트

```bash
cd measure

# 의존성 설치
pip install -r requirements.txt

# 테스트 이벤트 생성
cat > test_event.json << 'EOF'
{
  "body": "{\"image_url\": \"s3://hypermob-images/test/full_body.jpg\", \"height_cm\": 175.0, \"user_id\": \"test\"}"
}
EOF

# 핸들러 테스트
python -c "
import json
from lambda_handler import handler

with open('test_event.json') as f:
    event = json.load(f)

result = handler(event, None)
print(json.dumps(result, indent=2))
"
```

### recommend 함수 로컬 테스트

```bash
cd recommend

# 의존성 설치
pip install -r requirements.txt

# 테스트 이벤트 생성
cat > test_event.json << 'EOF'
{
  "body": "{\"height\": 175.0, \"upper_arm\": 31.0, \"forearm\": 26.0, \"thigh\": 51.0, \"calf\": 36.0, \"torso\": 61.0}"
}
EOF

# 핸들러 테스트
python -c "
import json
from lambda_handler import handler

with open('test_event.json') as f:
    event = json.load(f)

result = handler(event, None)
print(json.dumps(result, indent=2))
"
```

## Docker 로컬 빌드 및 테스트

### measure 함수

```bash
cd measure

# 이미지 빌드
docker build -t hypermob-measure .

# 로컬 실행
docker run -p 9000:8080 hypermob-measure

# 다른 터미널에서 테스트
curl -XPOST "http://localhost:9000/2015-03-31/functions/function/invocations" \
  -d '{
    "body": "{\"image_url\": \"s3://hypermob-images/test/full_body.jpg\", \"height_cm\": 175.0}"
  }'
```

### recommend 함수

```bash
cd recommend

# 이미지 빌드
docker build -t hypermob-recommend .

# 로컬 실행
docker run -p 9000:8080 hypermob-recommend

# 다른 터미널에서 테스트
curl -XPOST "http://localhost:9000/2015-03-31/functions/function/invocations" \
  -d '{
    "body": "{\"height\": 175.0, \"upper_arm\": 31.0, \"forearm\": 26.0, \"thigh\": 51.0, \"calf\": 36.0, \"torso\": 61.0}"
  }'
```

## 수동 업데이트

### 코드만 업데이트

```bash
# 이미지 리빌드
cd measure
docker build -t hypermob-measure .

# ECR에 푸시
ACCOUNT_ID=$(aws sts get-caller-identity --query Account --output text)
AWS_REGION=ap-northeast-2

docker tag hypermob-measure:latest $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-measure:latest
docker push $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-measure:latest

# Lambda 업데이트
aws lambda update-function-code \
  --function-name hypermob-measure \
  --image-uri $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-measure:latest
```

### 환경 변수 업데이트

```bash
aws lambda update-function-configuration \
  --function-name hypermob-measure \
  --environment "Variables={RESULT_BUCKET=hypermob-results,AWS_REGION=ap-northeast-2,NEW_VAR=value}"
```

### 메모리/타임아웃 조정

```bash
aws lambda update-function-configuration \
  --function-name hypermob-measure \
  --memory-size 3008 \
  --timeout 300
```

## 모니터링

### CloudWatch Logs 확인

```bash
# 최근 로그 스트림 보기
aws logs tail /aws/lambda/hypermob-measure --follow

# 에러 로그만 필터링
aws logs filter-log-events \
  --log-group-name /aws/lambda/hypermob-measure \
  --filter-pattern "ERROR"
```

### Lambda 메트릭 확인

```bash
# 실행 횟수
aws cloudwatch get-metric-statistics \
  --namespace AWS/Lambda \
  --metric-name Invocations \
  --dimensions Name=FunctionName,Value=hypermob-measure \
  --start-time 2025-10-26T00:00:00Z \
  --end-time 2025-10-26T23:59:59Z \
  --period 3600 \
  --statistics Sum

# 에러율
aws cloudwatch get-metric-statistics \
  --namespace AWS/Lambda \
  --metric-name Errors \
  --dimensions Name=FunctionName,Value=hypermob-measure \
  --start-time 2025-10-26T00:00:00Z \
  --end-time 2025-10-26T23:59:59Z \
  --period 3600 \
  --statistics Sum
```

## 비용 최적화 팁

1. **메모리 최적화**: CloudWatch Logs에서 실제 메모리 사용량 확인 후 조정
2. **Provisioned Concurrency**: Cold Start가 문제되면 예약 동시성 사용
3. **S3 Lifecycle**: 오래된 이미지 자동 삭제 정책 설정
4. **API Gateway 캐싱**: 동일 요청 캐싱으로 Lambda 호출 감소

## 트러블슈팅

### Lambda 타임아웃

```bash
# 타임아웃 증가 (최대 900초)
aws lambda update-function-configuration \
  --function-name hypermob-measure \
  --timeout 600
```

### 메모리 부족

```bash
# 메모리 증가 (최대 10240 MB)
aws lambda update-function-configuration \
  --function-name hypermob-measure \
  --memory-size 5120
```

### S3 권한 오류

IAM 역할에 S3 권한이 있는지 확인:

```bash
aws iam get-role-policy \
  --role-name HypermobLambdaExecutionRole \
  --policy-name HypermobLambdaPolicy
```

### ECR 푸시 실패

ECR 로그인 다시 시도:

```bash
aws ecr get-login-password --region ap-northeast-2 | \
  docker login --username AWS --password-stdin \
  $(aws sts get-caller-identity --query Account --output text).dkr.ecr.ap-northeast-2.amazonaws.com
```

## 추가 리소스

- [상세 배포 가이드](DEPLOYMENT.md)
- [API Gateway OpenAPI 스펙](openapi.yml)
- [AWS Lambda 개발자 가이드](https://docs.aws.amazon.com/lambda/)
- [MediaPipe 문서](https://google.github.io/mediapipe/)

## 라이선스

Copyright (c) 2025 HYPERMOB Platform Team
