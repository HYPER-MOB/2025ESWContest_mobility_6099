# HYPERMOB Platform Deployment Scripts

이 폴더에는 HYPERMOB 플랫폼을 AWS에 배포하기 위한 스크립트들이 포함되어 있습니다.

## 주요 스크립트

### 1. Deploy-Unified-Platform.ps1
**통합 플랫폼 배포 스크립트**

Platform-Application과 Platform-AI의 모든 Lambda 함수를 하나의 API Gateway에 배포합니다.

```powershell
# 기본 배포
.\Deploy-Unified-Platform.ps1

# 데이터베이스 초기화 건너뛰기
.\Deploy-Unified-Platform.ps1 -SkipDatabase

# Dry Run (실제 배포 없이 확인만)
.\Deploy-Unified-Platform.ps1 -DryRun

# 사용자 정의 설정 파일
.\Deploy-Unified-Platform.ps1 -ConfigFile my-config.json
```

**배포 항목:**
- 16개 Lambda 함수 (13개 Node.js + 3개 Python)
- API Gateway 생성 및 구성
- IAM Role 및 권한 설정
- 데이터베이스 스키마 생성 및 시드 데이터 삽입

---

### 2. Get-RDSNetworkConfig.ps1
**RDS 네트워크 설정 자동 감지 헬퍼**

RDS 인스턴스의 VPC/Subnet/Security Group 정보를 자동으로 가져와서 `deploy-config.json`을 생성합니다.

---

### 3. Test-Deployment.ps1
**배포 사전 점검 스크립트**

배포하기 전에 모든 요구사항이 충족되었는지 확인합니다.

```powershell
# 배포 준비 상태 확인
.\Test-Deployment.ps1
```

**확인 항목:**
- AWS CLI 설치 및 설정
- deploy-config.json 유효성
- Lambda 함수 디렉토리 및 핸들러
- Node.js/Python 설치
- MySQL 클라이언트
- 데이터베이스 스크립트

---

### 4. Test-ApiEndpoints.ps1
**API 엔드포인트 검증 스크립트**

배포된 API의 모든 엔드포인트를 자동으로 테스트합니다.

```powershell
# API URL 자동 감지
.\Test-ApiEndpoints.ps1

# 수동으로 API URL 지정
.\Test-ApiEndpoints.ps1 -ApiUrl "https://xxx.execute-api.us-east-1.amazonaws.com/v1"

# 상세 로그 출력 (Request/Response Body 포함)
.\Test-ApiEndpoints.ps1 -DetailedOutput
```

**테스트 항목:**
- ✅ Health Check
- ✅ User Management (사용자 등록)
- ✅ Vehicle Management (차량 목록)
- ✅ Booking Management (예약 생성/조회)
- ✅ Authentication (NFC/BLE 인증)
- ✅ Auth Session (세션 관리)
- ✅ User Profile (프로필 조회)
- ✅ Vehicle Control (차량 설정 적용)
- ⊘ Face Registration (수동 테스트 필요)
- ⊘ AI Services (S3 이미지 필요)

```powershell
# RDS 인스턴스 ID로 조회
.\Get-RDSNetworkConfig.ps1 -DBIdentifier hypermob-db

# DB 호스트 주소로 조회
.\Get-RDSNetworkConfig.ps1 -DBHost hypermob-db.abc123.us-east-1.rds.amazonaws.com

# 사용 가능한 RDS 인스턴스 목록 보기
.\Get-RDSNetworkConfig.ps1
```

**출력 정보:**
- RDS 인스턴스 상세 정보
- VPC 및 Subnet 목록
- Security Group 목록
- MySQL 포트(3306) 접근 가능 여부
- 자동 생성된 `deploy-config.json` (선택 사항)

---

## 빠른 시작

### 1단계: RDS 네트워크 설정 가져오기

```powershell
cd Platform-Application/scripts

# RDS 인스턴스 목록 확인
.\Get-RDSNetworkConfig.ps1

# 특정 RDS의 설정 가져오기
.\Get-RDSNetworkConfig.ps1 -DBIdentifier your-db-instance-id
```

스크립트가 `deploy-config.json`을 자동으로 생성할지 물어봅니다:
- **Y**: 자동 생성 (DB 비밀번호만 나중에 입력)
- **N**: 수동으로 생성

### 2단계: DB 비밀번호 설정

`deploy-config.json`을 열고 `DB_PASSWORD`를 실제 비밀번호로 변경:

```json
{
  "LAMBDA_SUBNET_IDS": "subnet-abc123,subnet-def456",
  "LAMBDA_SECURITY_GROUPS": "sg-xyz789",
  "DB_HOST": "hypermob-db.abc123.us-east-1.rds.amazonaws.com",
  "DB_PASSWORD": "YOUR_ACTUAL_PASSWORD_HERE",  // ← 이 부분만 변경
  "DB_USER": "admin",
  "DB_NAME": "hypermob"
}
```

### 3단계: 통합 배포 실행

```powershell
.\Deploy-Unified-Platform.ps1
```

배포가 완료되면 API Gateway 엔드포인트가 출력됩니다:
```
https://[API-ID].execute-api.us-east-1.amazonaws.com/v1
```

### 4단계: API Gateway 완성

```bash
# OpenAPI 스펙 import
aws apigateway put-rest-api \
  --rest-api-id [YOUR-API-ID] \
  --mode overwrite \
  --body file://../docs/openapi-unified.yaml

# API 배포
aws apigateway create-deployment \
  --rest-api-id [YOUR-API-ID] \
  --stage-name v1
```

### 5단계: 테스트

```bash
curl https://[YOUR-API-ID].execute-api.us-east-1.amazonaws.com/v1/health
```

---

## 설정 파일

### deploy-config.json

**필수 설정:**
```json
{
  "LAMBDA_SUBNET_IDS": "subnet-xxx,subnet-yyy",  // Lambda VPC Subnet
  "LAMBDA_SECURITY_GROUPS": "sg-zzz",             // Lambda Security Group
  "DB_HOST": "db.xxx.rds.amazonaws.com",          // RDS 엔드포인트
  "DB_PASSWORD": "your-password"                  // DB 비밀번호
}
```

**선택 설정 (기본값 사용 가능):**
```json
{
  "AWS_REGION": "us-east-1",      // 자동 감지됨
  "DB_USER": "admin",              // 기본값: admin
  "DB_NAME": "hypermob",           // 기본값: hypermob
  "API_NAME": "hypermob-unified-api"  // 기본값: hypermob-unified-api
}
```

### deploy-config.template.json

설정 파일 템플릿입니다. 복사해서 사용:
```bash
cp deploy-config.template.json deploy-config.json
```

---

## 트러블슈팅

### RDS 연결 실패

**증상:**
```
ERROR: Unable to connect to database
```

**해결:**
```powershell
# Security Group 규칙 확인
.\Get-RDSNetworkConfig.ps1 -DBIdentifier your-db-id
```

출력에서 다음을 확인:
- ✓ Found MySQL port (3306) rule → 정상
- ⚠ No MySQL (3306) inbound rule found → Security Group 규칙 추가 필요

**Security Group 규칙 추가:**
```bash
aws ec2 authorize-security-group-ingress \
  --group-id sg-xxx \
  --protocol tcp \
  --port 3306 \
  --source-group sg-yyy
```

### Lambda 함수 배포 실패

**증상:**
```
ERROR: Function path not found
```

**해결:**
```bash
# 함수 디렉토리 확인
ls ../lambdas/

# 누락된 함수가 있다면 Platform-AI에서 복사
cp -r ../../Platform-AI/lambdas/facedata ../lambdas/
```

### Python Lambda 함수 오류

**증상:**
```
ERROR: No module named 'cv2'
```

**해결:**
```bash
cd ../lambdas/measure
pip install -r requirements.txt -t .
```

---

## 폴더 구조

```
scripts/
├── Deploy-Unified-Platform.ps1      # 메인 배포 스크립트
├── Get-RDSNetworkConfig.ps1         # RDS 설정 헬퍼
├── deploy-config.template.json      # 설정 템플릿
├── deploy-config.json               # 실제 설정 (gitignore됨)
└── README.md                        # 이 파일
```

---

## 추가 리소스

- **상세 배포 가이드**: [../DEPLOYMENT_GUIDE.md](../DEPLOYMENT_GUIDE.md)
- **API 문서**: [../docs/openapi-unified.yaml](../docs/openapi-unified.yaml)
- **데이터베이스 스키마**: [../database/scripts/unified-schema.sql](../database/scripts/unified-schema.sql)
- **시드 데이터**: [../database/scripts/seeds.sql](../database/scripts/seeds.sql)

---

## 지원

문제가 발생하면 GitHub Issues에 등록해주세요:
https://github.com/hypermob/platform/issues
