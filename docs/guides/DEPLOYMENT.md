# HYPERMOB Platform AI - AWS Lambda 배포 가이드

## 목차
1. [아키텍처 개요](#아키텍처-개요)
2. [사전 준비사항](#사전-준비사항)
3. [S3 버킷 설정](#s3-버킷-설정)
4. [Lambda 함수 배포](#lambda-함수-배포)
5. [API Gateway 설정](#api-gateway-설정)
6. [배포 자동화](#배포-자동화)
7. [모니터링 및 로깅](#모니터링-및-로깅)
8. [비용 최적화](#비용-최적화)
9. [트러블슈팅](#트러블슈팅)

---

## 아키텍처 개요

```
┌──────────────┐         ┌─────────────────┐         ┌──────────────┐
│              │         │                 │         │              │
│   Client     │────────▶│  API Gateway    │────────▶│   Lambda     │
│  (Mobile/Web)│         │  (REST API)     │         │  Functions   │
│              │         │                 │         │              │
└──────────────┘         └─────────────────┘         └──────┬───────┘
                                                             │
                         ┌─────────────────┐                │
                         │                 │◀───────────────┘
                         │   S3 Buckets    │
                         │  - Images       │
                         │  - Models       │
                         │  - Results      │
                         └─────────────────┘
```

### 주요 컴포넌트

1. **API Gateway**: REST API 엔드포인트 제공
2. **Lambda Functions**:
   - `measure-function`: 신체 측정 (MediaPipe + OpenCV)
   - `recommend-function`: 차량 설정 추천 (scikit-learn)
3. **S3 Buckets**:
   - `hypermob-images`: 사용자 이미지 저장
   - `hypermob-models`: ML 모델 파일 저장
   - `hypermob-results`: 측정 결과 이미지 저장

---

## 사전 준비사항

### 1. AWS CLI 설치 및 설정

```bash
# AWS CLI 설치 확인
aws --version

# AWS 자격증명 설정
aws configure
# AWS Access Key ID: YOUR_ACCESS_KEY
# AWS Secret Access Key: YOUR_SECRET_KEY
# Default region name: ap-northeast-2
# Default output format: json
```

### 2. 필요한 도구 설치

```bash
# Python 3.9+ 설치 확인
python --version

# 필요한 Python 패키지 설치
pip install boto3 awscli

# Docker 설치 (Lambda 컨테이너 이미지 빌드용)
docker --version
```

### 3. IAM 권한 설정

배포를 위해 다음 권한이 필요합니다:
- `AmazonS3FullAccess
- `AWSLambda_FullAccess
- `AmazonAPIGatewayAdministrator
- `IAMFullAccess` (역할 생성용)
- `CloudWatchLogsFullAccess

---

## S3 버킷 설정

### 1. S3 버킷 생성

```bash
# 리전 설정
AWS_REGION="ap-northeast-2"

# 이미지 저장용 버킷
aws s3 mb s3://hypermob-images --region $AWS_REGION

# 모델 파일 저장용 버킷
aws s3 mb s3://hypermob-models --region $AWS_REGION

# 결과 이미지 저장용 버킷
aws s3 mb s3://hypermob-results --region $AWS_REGION
```

### 2. 버킷 정책 설정

**hypermob-images** 버킷 정책 (사용자 업로드용):

```bash
cat > /tmp/images-bucket-policy.json << 'EOF'
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Sid": "AllowLambdaRead",
      "Effect": "Allow",
      "Principal": {
        "Service": "lambda.amazonaws.com"
      },
      "Action": [
        "s3:GetObject",
        "s3:GetObjectVersion"
      ],
      "Resource": "arn:aws:s3:::hypermob-images/*"
    },
    {
      "Sid": "AllowUserUpload",
      "Effect": "Allow",
      "Principal": "*",
      "Action": "s3:PutObject",
      "Resource": "arn:aws:s3:::hypermob-images/uploads/*",
      "Condition": {
        "StringLike": {
          "s3:x-amz-server-side-encryption": "AES256"
        }
      }
    }
  ]
}
EOF

aws s3api put-bucket-policy --bucket hypermob-images --policy file:///tmp/images-bucket-policy.json
```

**hypermob-results** 버킷 정책 (Lambda 쓰기 + 사용자 읽기):

```bash
cat > /tmp/results-bucket-policy.json << 'EOF'
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Sid": "AllowLambdaWrite",
      "Effect": "Allow",
      "Principal": {
        "Service": "lambda.amazonaws.com"
      },
      "Action": [
        "s3:PutObject",
        "s3:PutObjectAcl"
      ],
      "Resource": "arn:aws:s3:::hypermob-results/*"
    },
    {
      "Sid": "AllowPublicRead",
      "Effect": "Allow",
      "Principal": "*",
      "Action": "s3:GetObject",
      "Resource": "arn:aws:s3:::hypermob-results/*"
    }
  ]
}
EOF

aws s3api put-bucket-policy --bucket hypermob-results --policy file:///tmp/results-bucket-policy.json
```

### 3. CORS 설정 (클라이언트 접근용)

```bash
cat > /tmp/cors-config.json << 'EOF'
{
  "CORSRules": [
    {
      "AllowedOrigins": ["*"],
      "AllowedMethods": ["GET", "PUT", "POST", "DELETE"],
      "AllowedHeaders": ["*"],
      "MaxAgeSeconds": 3000
    }
  ]
}
EOF

aws s3api put-bucket-cors --bucket hypermob-images --cors-configuration file:///tmp/cors-config.json
aws s3api put-bucket-cors --bucket hypermob-results --cors-configuration file:///tmp/cors-config.json
```

### 4. 수명 주기 정책 설정 (비용 절감)

```bash
cat > /tmp/lifecycle-policy.json << 'EOF'
{
  "Rules": [
    {
      "Id": "DeleteOldUploads",
      "Status": "Enabled",
      "Prefix": "uploads/",
      "Expiration": {
        "Days": 7
      }
    },
    {
      "Id": "TransitionToIA",
      "Status": "Enabled",
      "Prefix": "results/",
      "Transitions": [
        {
          "Days": 30,
          "StorageClass": "STANDARD_IA"
        },
        {
          "Days": 90,
          "StorageClass": "GLACIER"
        }
      ]
    }
  ]
}
EOF

aws s3api put-bucket-lifecycle-configuration --bucket hypermob-images --lifecycle-configuration file:///tmp/lifecycle-policy.json
```

---

## Lambda 함수 배포

### 방법 1: Docker 컨테이너 이미지 (권장)

Lambda에서 OpenCV, MediaPipe 같은 대용량 라이브러리를 사용하려면 컨테이너 이미지 방식이 적합합니다.

#### 1. ECR 리포지토리 생성

```bash
aws ecr create-repository --repository-name hypermob-measure --region $AWS_REGION
aws ecr create-repository --repository-name hypermob-recommend --region $AWS_REGION
```

#### 2. Dockerfile 생성 - measure 함수

`measure/Dockerfile`:

```dockerfile
FROM public.ecr.aws/lambda/python:3.9

# 시스템 의존성 설치
RUN yum install -y \
    mesa-libGL \
    libgomp \
    && yum clean all

# Python 의존성 설치
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# 함수 코드 복사
COPY measure.py ${LAMBDA_TASK_ROOT}/
COPY lambda_measure.py ${LAMBDA_TASK_ROOT}/

# Lambda 핸들러 설정
CMD ["lambda_measure.handler"]
```

`measure/requirements.txt`:

```
opencv-python-headless==4.8.1.78
mediapipe==0.10.8
numpy==1.24.3
boto3==1.28.85
pillow==10.1.0
```

`measure/lambda_measure.py`:

```python
import json
import os
import boto3
import cv2
import numpy as np
from urllib.parse import urlparse
import mediapipe as mp

s3_client = boto3.client('s3')
mp_pose = mp.solutions.pose

def download_from_s3(s3_url):
    """S3 URL에서 이미지 다운로드"""
    parsed = urlparse(s3_url)
    bucket = parsed.netloc
    key = parsed.path.lstrip('/')

    local_path = f"/tmp/{os.path.basename(key)}"
    s3_client.download_file(bucket, key, local_path)
    return local_path

def upload_to_s3(local_path, bucket, key):
    """S3에 파일 업로드"""
    s3_client.upload_file(local_path, bucket, key, ExtraArgs={'ACL': 'public-read'})
    return f"https://{bucket}.s3.amazonaws.com/{key}"

def measure_body(image_path, height_cm):
    """measure.py의 핵심 로직을 함수화"""
    # measure.py의 코드를 여기에 통합
    # (기존 measure.py 코드를 함수로 리팩토링)

    img = cv2.imread(image_path)
    if img is None:
        raise FileNotFoundError(f"Unable to find image: {image_path}")

    h, w = img.shape[:2]

    with mp_pose.Pose(static_image_mode=True, model_complexity=2) as pose:
        img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        res = pose.process(img_rgb)

    if not res.pose_landmarks:
        raise RuntimeError("Unable to find pose landmarks")

    # 측정 로직 (measure.py 참조)
    # ... (상세 구현은 measure.py 코드 활용)

    # 결과 이미지 생성
    overlay_path = "/tmp/measurement_overlay.jpg"
    cv2.imwrite(overlay_path, img)

    return {
        "arm_cm": 65.3,  # 실제 측정값
        "leg_cm": 92.1,
        "torso_cm": 61.5,
        "output_image": overlay_path
    }

def handler(event, context):
    """Lambda 핸들러"""
    try:
        body = json.loads(event['body']) if isinstance(event.get('body'), str) else event

        image_url = body['image_url']
        height_cm = float(body['height_cm'])
        user_id = body.get('user_id', 'unknown')

        # S3에서 이미지 다운로드
        local_image = download_from_s3(image_url)

        # 측정 수행
        result = measure_body(local_image, height_cm)

        # 결과 이미지 S3에 업로드
        result_key = f"results/{user_id}/measurement_{os.urandom(8).hex()}.jpg"
        result_url = upload_to_s3(result['output_image'], 'hypermob-results', result_key)

        return {
            'statusCode': 200,
            'headers': {'Content-Type': 'application/json'},
            'body': json.dumps({
                'status': 'success',
                'data': {
                    'arm_cm': result['arm_cm'],
                    'leg_cm': result['leg_cm'],
                    'torso_cm': result['torso_cm'],
                    'output_image_url': result_url
                }
            })
        }

    except Exception as e:
        return {
            'statusCode': 500,
            'headers': {'Content-Type': 'application/json'},
            'body': json.dumps({
                'status': 'error',
                'error': {
                    'code': 'INTERNAL_ERROR',
                    'message': str(e)
                }
            })
        }
```

#### 3. Dockerfile 생성 - recommend 함수

`recommend/Dockerfile`:

```dockerfile
FROM public.ecr.aws/lambda/python:3.9

# Python 의존성 설치
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# 함수 코드 및 모델 복사
COPY recommend.py ${LAMBDA_TASK_ROOT}/
COPY lambda_recommend.py ${LAMBDA_TASK_ROOT}/
COPY json_template.json ${LAMBDA_TASK_ROOT}/

# 모델 학습 및 저장
RUN python ${LAMBDA_TASK_ROOT}/recommend.py

CMD ["lambda_recommend.handler"]
```

`recommend/requirements.txt`:

```
numpy==1.24.3
pandas==2.0.3
scikit-learn==1.3.2
joblib==1.3.2
boto3==1.28.85
```

`recommend/lambda_recommend.py`:

```python
import json
import numpy as np
from recommend import load_model, predict_car_fit

# 모델 초기화 (Lambda 컨테이너 시작 시 한 번만 로드)
try:
    model = load_model("/var/task/car_fit_model.joblib")
    feature_cols = ["height", "upper_arm", "forearm", "thigh", "calf", "torso"]
except Exception as e:
    print(f"Model loading error: {e}")
    model = None

def handler(event, context):
    """Lambda 핸들러"""
    try:
        if model is None:
            raise RuntimeError("Model not loaded")

        body = json.loads(event['body']) if isinstance(event.get('body'), str) else event

        # 입력 검증
        required_fields = ["height", "upper_arm", "forearm", "thigh", "calf", "torso"]
        for field in required_fields:
            if field not in body:
                raise ValueError(f"Missing required field: {field}")

        user_measure = {k: float(body[k]) for k in required_fields}

        # 예측 수행
        prediction = predict_car_fit(model, user_measure, feature_cols)

        return {
            'statusCode': 200,
            'headers': {'Content-Type': 'application/json'},
            'body': json.dumps({
                'status': 'success',
                'data': prediction
            })
        }

    except ValueError as e:
        return {
            'statusCode': 400,
            'headers': {'Content-Type': 'application/json'},
            'body': json.dumps({
                'status': 'error',
                'error': {
                    'code': 'INVALID_INPUT',
                    'message': str(e)
                }
            })
        }
    except Exception as e:
        return {
            'statusCode': 500,
            'headers': {'Content-Type': 'application/json'},
            'body': json.dumps({
                'status': 'error',
                'error': {
                    'code': 'INTERNAL_ERROR',
                    'message': str(e)
                }
            })
        }
```

#### 4. Docker 이미지 빌드 및 푸시

```bash
# ECR 로그인
aws ecr get-login-password --region $AWS_REGION | docker login --username AWS --password-stdin $(aws sts get-caller-identity --query Account --output text).dkr.ecr.$AWS_REGION.amazonaws.com

# measure 함수 빌드 및 푸시
cd measure
docker build -t hypermob-measure .
docker tag hypermob-measure:latest $(aws sts get-caller-identity --query Account --output text).dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-measure:latest
docker push $(aws sts get-caller-identity --query Account --output text).dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-measure:latest

# recommend 함수 빌드 및 푸시
cd ../recommend
docker build -t hypermob-recommend .
docker tag hypermob-recommend:latest $(aws sts get-caller-identity --query Account --output text).dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-recommend:latest
docker push $(aws sts get-caller-identity --query Account --output text).dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-recommend:latest
```

#### 5. IAM 역할 생성

```bash
# Lambda 실행 역할 생성
cat > /tmp/lambda-trust-policy.json << 'EOF'
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Principal": {
        "Service": "lambda.amazonaws.com"
      },
      "Action": "sts:AssumeRole"
    }
  ]
}
EOF

aws iam create-role \
  --role-name HypermobLambdaExecutionRole \
  --assume-role-policy-document file:///tmp/lambda-trust-policy.json

# S3 접근 정책 추가
cat > /tmp/lambda-s3-policy.json << 'EOF'
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "s3:GetObject",
        "s3:PutObject",
        "s3:PutObjectAcl"
      ],
      "Resource": [
        "arn:aws:s3:::hypermob-images/*",
        "arn:aws:s3:::hypermob-models/*",
        "arn:aws:s3:::hypermob-results/*"
      ]
    },
    {
      "Effect": "Allow",
      "Action": [
        "logs:CreateLogGroup",
        "logs:CreateLogStream",
        "logs:PutLogEvents"
      ],
      "Resource": "arn:aws:logs:*:*:*"
    }
  ]
}
EOF

aws iam put-role-policy \
  --role-name HypermobLambdaExecutionRole \
  --policy-name HypermobS3Access \
  --policy-document file:///tmp/lambda-s3-policy.json
```

#### 6. Lambda 함수 생성

```bash
ACCOUNT_ID=$(aws sts get-caller-identity --query Account --output text)

# measure 함수 생성
aws lambda create-function \
  --function-name hypermob-measure \
  --package-type Image \
  --code ImageUri=$ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-measure:latest \
  --role arn:aws:iam::$ACCOUNT_ID:role/HypermobLambdaExecutionRole \
  --timeout 300 \
  --memory-size 3008 \
  --region $AWS_REGION

# recommend 함수 생성
aws lambda create-function \
  --function-name hypermob-recommend \
  --package-type Image \
  --code ImageUri=$ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-recommend:latest \
  --role arn:aws:iam::$ACCOUNT_ID:role/HypermobLambdaExecutionRole \
  --timeout 60 \
  --memory-size 1024 \
  --region $AWS_REGION
```

### 방법 2: ZIP 파일 배포 (경량 함수용)

recommend 함수는 ZIP 방식도 가능합니다:

```bash
# 의존성 설치
mkdir -p recommend-package
pip install -r recommend/requirements.txt -t recommend-package/

# 코드 복사
cp recommend.py recommend-package/
cp lambda_recommend.py recommend-package/
cp json_template.json recommend-package/

# 모델 사전 학습
cd recommend-package
python recommend.py
cd ..

# ZIP 생성
cd recommend-package
zip -r ../recommend-function.zip .
cd ..

# Lambda 함수 생성
aws lambda create-function \
  --function-name hypermob-recommend \
  --runtime python3.9 \
  --role arn:aws:iam::$ACCOUNT_ID:role/HypermobLambdaExecutionRole \
  --handler lambda_recommend.handler \
  --zip-file fileb://recommend-function.zip \
  --timeout 60 \
  --memory-size 512
```

---

## API Gateway 설정

### 1. OpenAPI 스펙으로 API 생성

```bash
# API Gateway 생성
aws apigateway import-rest-api \
  --body file://openapi.yml \
  --region $AWS_REGION \
  --fail-on-warnings \
  --output json > /tmp/api-output.json

API_ID=$(jq -r '.id' /tmp/api-output.json)
echo "API ID: $API_ID"
```

### 2. Lambda 통합 설정

```bash
# measure 함수 통합
MEASURE_URI="arn:aws:apigateway:$AWS_REGION:lambda:path/2015-03-31/functions/arn:aws:lambda:$AWS_REGION:$ACCOUNT_ID:function:hypermob-measure/invocations"

aws apigateway put-integration \
  --rest-api-id $API_ID \
  --resource-id $(aws apigateway get-resources --rest-api-id $API_ID --query "items[?path=='/measure'].id" --output text) \
  --http-method POST \
  --type AWS_PROXY \
  --integration-http-method POST \
  --uri $MEASURE_URI

# recommend 함수 통합
RECOMMEND_URI="arn:aws:apigateway:$AWS_REGION:lambda:path/2015-03-31/functions/arn:aws:lambda:$AWS_REGION:$ACCOUNT_ID:function:hypermob-recommend/invocations"

aws apigateway put-integration \
  --rest-api-id $API_ID \
  --resource-id $(aws apigateway get-resources --rest-api-id $API_ID --query "items[?path=='/recommend'].id" --output text) \
  --http-method POST \
  --type AWS_PROXY \
  --integration-http-method POST \
  --uri $RECOMMEND_URI
```

### 3. Lambda 권한 부여

```bash
# API Gateway가 Lambda를 호출할 수 있도록 권한 부여
aws lambda add-permission \
  --function-name hypermob-measure \
  --statement-id apigateway-measure \
  --action lambda:InvokeFunction \
  --principal apigateway.amazonaws.com \
  --source-arn "arn:aws:execute-api:$AWS_REGION:$ACCOUNT_ID:$API_ID/*/*"

aws lambda add-permission \
  --function-name hypermob-recommend \
  --statement-id apigateway-recommend \
  --action lambda:InvokeFunction \
  --principal apigateway.amazonaws.com \
  --source-arn "arn:aws:execute-api:$AWS_REGION:$ACCOUNT_ID:$API_ID/*/*"
```

### 4. API 배포

```bash
# 스테이지 생성 및 배포
aws apigateway create-deployment \
  --rest-api-id $API_ID \
  --stage-name prod \
  --stage-description "Production deployment" \
  --description "Initial deployment"

# API 엔드포인트 확인
echo "API Endpoint: https://$API_ID.execute-api.$AWS_REGION.amazonaws.com/prod"
```

### 5. API Key 설정 (선택사항)

```bash
# API Key 생성
aws apigateway create-api-key \
  --name hypermob-api-key \
  --description "HYPERMOB Platform API Key" \
  --enabled \
  --output json > /tmp/api-key.json

API_KEY_ID=$(jq -r '.id' /tmp/api-key.json)
API_KEY_VALUE=$(jq -r '.value' /tmp/api-key.json)

# Usage Plan 생성
aws apigateway create-usage-plan \
  --name hypermob-usage-plan \
  --throttle rateLimit=100,burstLimit=200 \
  --quota limit=10000,period=MONTH \
  --api-stages apiId=$API_ID,stage=prod \
  --output json > /tmp/usage-plan.json

USAGE_PLAN_ID=$(jq -r '.id' /tmp/usage-plan.json)

# API Key와 Usage Plan 연결
aws apigateway create-usage-plan-key \
  --usage-plan-id $USAGE_PLAN_ID \
  --key-id $API_KEY_ID \
  --key-type API_KEY

echo "API Key: $API_KEY_VALUE"
```

---

## 배포 자동화

### GitHub Actions를 활용한 CI/CD

`.github/workflows/deploy.yml`:

```yaml
name: Deploy to AWS Lambda

on:
  push:
    branches:
      - main
  workflow_dispatch:

env:
  AWS_REGION: ap-northeast-2
  ECR_REGISTRY: ${{ secrets.AWS_ACCOUNT_ID }}.dkr.ecr.ap-northeast-2.amazonaws.com

jobs:
  deploy-measure:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout code
        uses: actions/checkout@v3

      - name: Configure AWS credentials
        uses: aws-actions/configure-aws-credentials@v2
        with:
          aws-access-key-id: ${{ secrets.AWS_ACCESS_KEY_ID }}
          aws-secret-access-key: ${{ secrets.AWS_SECRET_ACCESS_KEY }}
          aws-region: ${{ env.AWS_REGION }}

      - name: Login to Amazon ECR
        id: login-ecr
        uses: aws-actions/amazon-ecr-login@v1

      - name: Build and push measure image
        working-directory: ./measure
        run: |
          docker build -t hypermob-measure .
          docker tag hypermob-measure:latest $ECR_REGISTRY/hypermob-measure:latest
          docker push $ECR_REGISTRY/hypermob-measure:latest

      - name: Update Lambda function
        run: |
          aws lambda update-function-code \
            --function-name hypermob-measure \
            --image-uri $ECR_REGISTRY/hypermob-measure:latest

  deploy-recommend:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout code
        uses: actions/checkout@v3

      - name: Configure AWS credentials
        uses: aws-actions/configure-aws-credentials@v2
        with:
          aws-access-key-id: ${{ secrets.AWS_ACCESS_KEY_ID }}
          aws-secret-access-key: ${{ secrets.AWS_SECRET_ACCESS_KEY }}
          aws-region: ${{ env.AWS_REGION }}

      - name: Login to Amazon ECR
        id: login-ecr
        uses: aws-actions/amazon-ecr-login@v1

      - name: Build and push recommend image
        working-directory: ./recommend
        run: |
          docker build -t hypermob-recommend .
          docker tag hypermob-recommend:latest $ECR_REGISTRY/hypermob-recommend:latest
          docker push $ECR_REGISTRY/hypermob-recommend:latest

      - name: Update Lambda function
        run: |
          aws lambda update-function-code \
            --function-name hypermob-recommend \
            --image-uri $ECR_REGISTRY/hypermob-recommend:latest
```

### 로컬 배포 스크립트

`deploy.sh`:

```bash
#!/bin/bash

set -e

AWS_REGION="ap-northeast-2"
ACCOUNT_ID=$(aws sts get-caller-identity --query Account --output text)
ECR_REGISTRY="$ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com"

echo "========================================="
echo "HYPERMOB Platform AI - Deployment Script"
echo "========================================="
echo ""

# ECR 로그인
echo "[1/4] Logging in to ECR..."
aws ecr get-login-password --region $AWS_REGION | docker login --username AWS --password-stdin $ECR_REGISTRY

# measure 함수 배포
echo "[2/4] Building and deploying measure function..."
cd measure
docker build -t hypermob-measure .
docker tag hypermob-measure:latest $ECR_REGISTRY/hypermob-measure:latest
docker push $ECR_REGISTRY/hypermob-measure:latest
aws lambda update-function-code \
  --function-name hypermob-measure \
  --image-uri $ECR_REGISTRY/hypermob-measure:latest
cd ..

# recommend 함수 배포
echo "[3/4] Building and deploying recommend function..."
cd recommend
docker build -t hypermob-recommend .
docker tag hypermob-recommend:latest $ECR_REGISTRY/hypermob-recommend:latest
docker push $ECR_REGISTRY/hypermob-recommend:latest
aws lambda update-function-code \
  --function-name hypermob-recommend \
  --image-uri $ECR_REGISTRY/hypermob-recommend:latest
cd ..

# 배포 완료 대기
echo "[4/4] Waiting for deployment to complete..."
aws lambda wait function-updated --function-name hypermob-measure
aws lambda wait function-updated --function-name hypermob-recommend

echo ""
echo "✓ Deployment completed successfully!"
echo ""
echo "API Endpoint: https://$(aws apigateway get-rest-apis --query "items[?name=='HYPERMOB Platform AI API'].id" --output text).execute-api.$AWS_REGION.amazonaws.com/prod"
```

---

## 모니터링 및 로깅

### CloudWatch 대시보드 생성

```bash
cat > /tmp/dashboard.json << 'EOF'
{
  "widgets": [
    {
      "type": "metric",
      "properties": {
        "metrics": [
          ["AWS/Lambda", "Invocations", {"stat": "Sum", "label": "Measure Invocations"}],
          [".", ".", {"stat": "Sum", "label": "Recommend Invocations"}]
        ],
        "period": 300,
        "stat": "Sum",
        "region": "ap-northeast-2",
        "title": "Lambda Invocations"
      }
    },
    {
      "type": "metric",
      "properties": {
        "metrics": [
          ["AWS/Lambda", "Duration", {"stat": "Average", "label": "Measure Duration"}],
          [".", ".", {"stat": "Average", "label": "Recommend Duration"}]
        ],
        "period": 300,
        "stat": "Average",
        "region": "ap-northeast-2",
        "title": "Lambda Duration (ms)"
      }
    },
    {
      "type": "metric",
      "properties": {
        "metrics": [
          ["AWS/Lambda", "Errors", {"stat": "Sum", "label": "Measure Errors"}],
          [".", ".", {"stat": "Sum", "label": "Recommend Errors"}]
        ],
        "period": 300,
        "stat": "Sum",
        "region": "ap-northeast-2",
        "title": "Lambda Errors"
      }
    }
  ]
}
EOF

aws cloudwatch put-dashboard \
  --dashboard-name HypermobPlatformAI \
  --dashboard-body file:///tmp/dashboard.json
```

### CloudWatch Alarms 설정

```bash
# 에러율 알람
aws cloudwatch put-metric-alarm \
  --alarm-name hypermob-measure-high-errors \
  --alarm-description "Measure function error rate too high" \
  --metric-name Errors \
  --namespace AWS/Lambda \
  --statistic Sum \
  --period 300 \
  --threshold 10 \
  --comparison-operator GreaterThanThreshold \
  --evaluation-periods 2 \
  --dimensions Name=FunctionName,Value=hypermob-measure

# 실행 시간 알람
aws cloudwatch put-metric-alarm \
  --alarm-name hypermob-measure-high-duration \
  --alarm-description "Measure function duration too high" \
  --metric-name Duration \
  --namespace AWS/Lambda \
  --statistic Average \
  --period 300 \
  --threshold 30000 \
  --comparison-operator GreaterThanThreshold \
  --evaluation-periods 2 \
  --dimensions Name=FunctionName,Value=hypermob-measure
```

---

## 비용 최적화

### 1. Lambda 예약 동시성 (Reserved Concurrency)

트래픽이 예측 가능한 경우:

```bash
aws lambda put-function-concurrency \
  --function-name hypermob-measure \
  --reserved-concurrent-executions 10
```

### 2. CloudFront를 통한 캐싱

API Gateway 앞에 CloudFront 배치:

```bash
aws cloudfront create-distribution \
  --origin-domain-name $API_ID.execute-api.$AWS_REGION.amazonaws.com \
  --default-cache-behavior '{
    "TargetOriginId": "api-gateway",
    "ViewerProtocolPolicy": "redirect-to-https",
    "AllowedMethods": {
      "Quantity": 7,
      "Items": ["GET", "HEAD", "OPTIONS", "PUT", "POST", "PATCH", "DELETE"]
    },
    "CachedMethods": {
      "Quantity": 2,
      "Items": ["GET", "HEAD"]
    },
    "Compress": true,
    "DefaultTTL": 0
  }'
```

### 3. 비용 추정

월 10,000 요청 기준:
- Lambda (measure): 3GB, 30초 평균 → 약 $5
- Lambda (recommend): 1GB, 1초 평균 → 약 $0.2
- API Gateway: 10,000 요청 → 약 $0.04
- S3: 10GB 저장, 10,000 요청 → 약 $0.25
- **총 예상 비용: 약 $5.5/월**

---

## 트러블슈팅

### 1. Lambda 타임아웃

**문제**: measure 함수가 타임아웃 발생

**해결**:
```bash
aws lambda update-function-configuration \
  --function-name hypermob-measure \
  --timeout 300
```

### 2. 메모리 부족

**문제**: OpenCV/MediaPipe 실행 중 메모리 부족

**해결**:
```bash
aws lambda update-function-configuration \
  --function-name hypermob-measure \
  --memory-size 3008
```

### 3. S3 권한 오류

**문제**: Lambda에서 S3 접근 불가

**해결**:
```bash
# Lambda 실행 역할에 S3 권한 추가
aws iam attach-role-policy \
  --role-name HypermobLambdaExecutionRole \
  --policy-arn arn:aws:iam::aws:policy/AmazonS3FullAccess
```

### 4. Cold Start 최적화

**문제**: 첫 요청이 너무 느림

**해결**:
- Provisioned Concurrency 사용
```bash
aws lambda put-provisioned-concurrency-config \
  --function-name hypermob-measure \
  --provisioned-concurrent-executions 2 \
  --qualifier '$LATEST'
```

### 5. 로그 확인

```bash
# 최근 로그 스트림 확인
aws logs tail /aws/lambda/hypermob-measure --follow

# 에러 로그만 필터링
aws logs filter-log-events \
  --log-group-name /aws/lambda/hypermob-measure \
  --filter-pattern "ERROR"
```

---

## 테스트

### API 테스트 예시

```bash
# 헬스 체크
curl https://$API_ID.execute-api.$AWS_REGION.amazonaws.com/prod/health

# measure API 테스트
curl -X POST https://$API_ID.execute-api.$AWS_REGION.amazonaws.com/prod/measure \
  -H "Content-Type: application/json" \
  -H "X-API-Key: YOUR_API_KEY" \
  -d '{
    "image_url": "s3://hypermob-images/test/full_body.jpg",
    "height_cm": 175.0,
    "user_id": "test_user"
  }'

# recommend API 테스트
curl -X POST https://$API_ID.execute-api.$AWS_REGION.amazonaws.com/prod/recommend \
  -H "Content-Type: application/json" \
  -H "X-API-Key: YOUR_API_KEY" \
  -d '{
    "height": 175.0,
    "upper_arm": 31.0,
    "forearm": 26.0,
    "thigh": 51.0,
    "calf": 36.0,
    "torso": 61.0,
    "user_id": "test_user"
  }'
```

---

## 참고 자료

- [AWS Lambda 개발자 가이드](https://docs.aws.amazon.com/lambda/)
- [API Gateway OpenAPI 확장](https://docs.aws.amazon.com/apigateway/latest/developerguide/api-gateway-swagger-extensions.html)
- [Lambda 컨테이너 이미지](https://docs.aws.amazon.com/lambda/latest/dg/images-create.html)
- [S3 버킷 정책](https://docs.aws.amazon.com/AmazonS3/latest/userguide/bucket-policies.html)
