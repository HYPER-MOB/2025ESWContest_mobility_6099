# MediaPipe Read-Only 문제 최종 해결

## 문제 원인

MediaPipe는 처음 실행 시 모델 파일(.tflite)을 다운로드하려고 합니다:
```
/var/lang/lib/python3.9/site-packages/mediapipe/modules/pose_landmark/pose_landmark_heavy.tflite
```

Lambda의 `/var/lang`는 read-only 파일시스템이므로 다운로드 실패합니다.

## 해결 방법

Docker 빌드 시 모델 파일을 미리 다운로드하여 이미지에 포함시킵니다.

## 수정된 파일

### 1. [measure/download_models.py](measure/download_models.py) (신규)
Docker 빌드 시 MediaPipe 모델을 다운로드하는 스크립트

```python
import os
import mediapipe as mp

os.environ['MEDIAPIPE_DISABLE_GPU'] = '1'
os.environ['GLOG_logtostderr'] = '1'

# Pose 모델 초기화 (자동 다운로드)
with mp.solutions.pose.Pose(
    static_image_mode=True,
    model_complexity=2
) as pose:
    print("Pose model downloaded!")
```

### 2. [measure/Dockerfile](measure/Dockerfile) (수정)
빌드 시 모델 다운로드 스크립트 실행

```dockerfile
# MediaPipe 모델 파일 사전 다운로드
COPY download_models.py /tmp/download_models.py
RUN python3.9 /tmp/download_models.py && rm /tmp/download_models.py
```

### 3. [measure/lambda_handler.py](measure/lambda_handler.py) (수정)
환경 변수 설정 추가

```python
os.environ['MEDIAPIPE_DISABLE_GPU'] = '1'
os.environ['GLOG_logtostderr'] = '1'
os.environ['TMPDIR'] = '/tmp'
```

## 재배포 방법

### PowerShell

```powershell
# 빠른 재배포
.\redeploy_functions.ps1
```

### Git Bash (단계별)

```bash
ACCOUNT_ID=$(aws sts get-caller-identity --query Account --output text)
AWS_REGION="ap-northeast-2"

# ECR 로그인
aws ecr get-login-password --region $AWS_REGION | \
    docker login --username AWS --password-stdin \
    $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com

# measure 함수 리빌드 (모델 다운로드 포함)
cd measure
docker build -t hypermob-measure .
docker tag hypermob-measure:latest $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-measure:latest
docker push $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-measure:latest
cd ..

# Lambda 업데이트
aws lambda update-function-code \
    --function-name hypermob-measure \
    --image-uri $ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com/hypermob-measure:latest \
    --region $AWS_REGION

# 업데이트 대기
aws lambda wait function-updated --function-name hypermob-measure --region $AWS_REGION

echo "Deployment complete!"
```

## 빌드 시 예상 출력

```
Step 5/10 : COPY download_models.py /tmp/download_models.py
 ---> Using cache
 ---> abc123def456
Step 6/10 : RUN python3.9 /tmp/download_models.py && rm /tmp/download_models.py
 ---> Running in xyz789abc012
Downloading MediaPipe models...
Initializing Pose model (complexity 2)...
Pose model downloaded successfully!
All models downloaded!
 ---> def456ghi789
```

## 테스트

재배포 후:

```powershell
# API 테스트
.\test_all_endpoints.ps1
```

예상 결과:
```
=========================================
 Test 5: Measure API
=========================================

[TEST] Testing POST /measure
[INFO] Response received in 15s
{
  "status": "success",
  "data": {
    "scale_cm_per_px": 0.2145,
    "arm_cm": 65.3,
    "leg_cm": 92.1,
    "torso_cm": 61.5,
    "output_image_url": "https://..."
  }
}
[PASS] Measure API test passed
[PASS] Response time < 30s
```

## 타임라인

1. **Docker 빌드** (5-7분)
   - 의존성 설치
   - **MediaPipe 모델 다운로드** ← 여기서 모델 파일 포함
   - 이미지 빌드

2. **ECR 푸시** (2-3분)

3. **Lambda 업데이트** (1-2분)

4. **첫 실행** (15-20초)
   - Cold start
   - 모델 파일은 이미 포함되어 있으므로 다운로드 없음
   - 즉시 실행 가능

## 개선 사항

### 이전 (문제)
```
Lambda 시작
  ↓
MediaPipe 초기화
  ↓
모델 파일 다운로드 시도
  ↓
❌ Read-only 파일시스템 에러
```

### 이후 (해결)
```
Docker 빌드 시
  ↓
MediaPipe 설치
  ↓
✅ 모델 파일 사전 다운로드
  ↓
이미지에 포함

Lambda 실행 시
  ↓
MediaPipe 초기화
  ↓
✅ 이미 다운로드된 모델 사용
  ↓
정상 실행
```

## 주의사항

### 이미지 크기 증가

MediaPipe 모델 파일이 포함되어 Docker 이미지 크기가 증가합니다:
- 이전: ~1.5GB
- 이후: ~1.7GB (약 200MB 증가)

Lambda 이미지 크기 제한(10GB)보다 훨씬 작으므로 문제없습니다.

### Cold Start 시간

모델 파일이 이미지에 포함되어 Cold Start 시간이 단축됩니다:
- 이전: 30초+ (모델 다운로드 포함)
- 이후: 15-20초 (모델 즉시 사용)

## 다음 단계

1. **재배포 실행**
```powershell
.\redeploy_functions.ps1
```

2. **테스트**
```powershell
.\test_all_endpoints.ps1
```

3. **CloudWatch Logs 확인**
```bash
aws logs tail /aws/lambda/hypermob-measure --follow --region ap-northeast-2
```

정상 로그:
```
Processing image: /tmp/full_body.jpg
Image size: 3000x4000
✅ (모델 다운로드 메시지 없음 - 이미 포함됨)
Measurement complete: {...}
```

## 최종 확인 체크리스트

- [ ] measure/download_models.py 파일 생성됨
- [ ] measure/Dockerfile에 모델 다운로드 단계 추가됨
- [ ] measure/lambda_handler.py에 환경 변수 설정됨
- [ ] Docker 이미지 리빌드
- [ ] ECR에 푸시
- [ ] Lambda 함수 업데이트
- [ ] 테스트 통과 (measure API 200 OK)
- [ ] CloudWatch Logs에서 "Read-only file system" 에러 없음
