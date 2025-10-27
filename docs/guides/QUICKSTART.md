# 빠른 배포 가이드

## Windows 사용자

### 방법 1: PowerShell (권장)

```powershell
# PowerShell 관리자 권한으로 실행
.\deploy.ps1
```

### 방법 2: Git Bash

```bash
# Git Bash에서 실행
./deploy.sh
```

## Linux/macOS 사용자

```bash
chmod +x deploy.sh
./deploy.sh
```

## 배포 과정

스크립트는 다음을 자동으로 수행합니다:

1. ✅ S3 버킷 생성 (images, models, results)
2. ✅ IAM 역할 생성 및 권한 설정
3. ✅ ECR 리포지토리 생성
4. ✅ Docker 이미지 빌드
5. ✅ ECR에 이미지 푸시
6. ✅ Lambda 함수 생성/업데이트
7. ✅ API Gateway 설정
8. ✅ Lambda 권한 부여
9. ✅ API 배포

## 에러 발생 시

### IAM 권한 부족

```bash
# 현재 사용자 권한 확인
aws iam get-user

# 필요한 권한:
# - IAMFullAccess
# - AmazonS3FullAccess
# - AWSLambda_FullAccess
# - AmazonAPIGatewayAdministrator
# - AmazonEC2ContainerRegistryFullAccess
```

### Docker 미설치

**Windows**: Docker Desktop 설치
```
https://www.docker.com/products/docker-desktop
```

**Linux**:
```bash
sudo apt-get update
sudo apt-get install docker.io
sudo systemctl start docker
```

### Git Bash에서 경로 에러

수정된 [deploy.sh](deploy.sh)를 사용하면 Windows 환경에서도 작동합니다.
`./temp` 폴더를 임시 디렉토리로 사용합니다.

## 테스트

배포 완료 후:

```bash
# Git Bash
./test_api.sh

# PowerShell
# curl 대신 Invoke-RestMethod 사용
$apiId = "your-api-id"
$region = "ap-northeast-2"
$url = "https://$apiId.execute-api.$region.amazonaws.com/prod/health"
Invoke-RestMethod -Uri $url
```

## 다음 단계

1. [DEPLOYMENT.md](DEPLOYMENT.md) - 상세 배포 문서
2. [README_LAMBDA.md](README_LAMBDA.md) - Lambda 함수 문서
3. CloudWatch 대시보드 설정
4. API Key 설정 (선택사항)
