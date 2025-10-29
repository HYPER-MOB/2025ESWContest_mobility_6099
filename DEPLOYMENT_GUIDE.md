# HYPERMOB Unified Platform - 배포 가이드

이 문서는 Platform-Application과 Platform-AI의 모든 Lambda 함수를 하나의 API Gateway에 통합 배포하는 방법을 설명합니다.

## 사전 요구사항

### 1. AWS CLI 설정
```bash
aws configure
# AWS Access Key ID: [your-access-key]
# AWS Secret Access Key: [your-secret-key]
# Default region name: us-east-1
# Default output format: json
```

### 2. 필수 도구
- **PowerShell** (Windows) 또는 **PowerShell Core** (Linux/Mac)
- **AWS CLI** v2.x
- **MySQL Client** (데이터베이스 초기화용)
- **Node.js** 18.x (Node.js Lambda 함수용)
- **Python** 3.11 (Python Lambda 함수용)

### 3. AWS 리소스
- **RDS MySQL** 인스턴스
- **VPC Subnets** (Lambda가 DB에 접근하기 위해 필요)
- **Security Groups** (Lambda와 RDS 간 통신 허용)

---

## 빠른 시작

### Step 1: 설정 파일 생성

`scripts/` 디렉토리로 이동:
```bash
cd Platform-Application/scripts
```

#### 옵션 A: 자동으로 RDS 설정 가져오기 (추천) 🚀

RDS 인스턴스의 네트워크 설정을 자동으로 가져옵니다:

```powershell
# RDS 인스턴스 ID로 조회
.\Get-RDSNetworkConfig.ps1 -DBIdentifier your-db-instance-id

# 또는 DB 호스트 주소로 조회
.\Get-RDSNetworkConfig.ps1 -DBHost your-db.abc123.us-east-1.rds.amazonaws.com

# 사용 가능한 RDS 인스턴스 목록 보기
.\Get-RDSNetworkConfig.ps1
```

이 스크립트는:
- ✅ RDS의 VPC/Subnet/Security Group 자동 감지
- ✅ MySQL 포트(3306) 접근 가능 여부 검증
- ✅ `deploy-config.json` 자동 생성 제안
- ✅ DB 비밀번호만 입력하면 배포 준비 완료!

#### 옵션 B: 수동으로 설정 파일 생성

설정 템플릿을 복사하여 `deploy-config.json` 생성:
```bash
cp deploy-config.template.json deploy-config.json
```

`deploy-config.json`을 편집하여 다음 값을 입력:
```json
{
  "LAMBDA_SUBNET_IDS": "subnet-xxxxxxxxx,subnet-yyyyyyyyy",
  "LAMBDA_SECURITY_GROUPS": "sg-zzzzzzzzz",
  "DB_HOST": "your-db.abc123.us-east-1.rds.amazonaws.com",
  "DB_PASSWORD": "YOUR_SECURE_PASSWORD_HERE"
}
```

**주의**: `AWS_REGION`과 `AWS_ACCOUNT_ID`는 자동으로 AWS CLI에서 감지됩니다!

### Step 2: 통합 배포 실행

PowerShell에서 배포 스크립트 실행:
```powershell
.\Deploy-Unified-Platform.ps1
```

배포 과정:
1. ✅ Lambda IAM Role 생성
2. ✅ 16개 Lambda 함수 배포 (13개 Node.js + 3개 Python)
3. ✅ API Gateway 생성
4. ✅ Lambda 함수에 API Gateway 권한 부여
5. ✅ 데이터베이스 스키마 생성
6. ✅ 더미 데이터 삽입

### Step 3: API Gateway 완성

OpenAPI 스펙을 import하여 API Gateway 완성:
```bash
aws apigateway put-rest-api \
  --rest-api-id [YOUR-API-ID] \
  --mode overwrite \
  --body file://../docs/openapi-unified.yaml
```

API를 `v1` 스테이지에 배포:
```bash
aws apigateway create-deployment \
  --rest-api-id [YOUR-API-ID] \
  --stage-name v1
```

### Step 4: API 엔드포인트 테스트

#### 옵션 A: 자동 테스트 스크립트 실행 (추천) 🚀

모든 엔드포인트를 자동으로 테스트:
```powershell
.\Test-ApiEndpoints.ps1
```

출력 예시:
```
========================================
API Endpoints Test Suite
========================================

Testing API: https://abc123.execute-api.us-east-1.amazonaws.com/v1

========================================
System Tests
========================================

[1] Testing: Health Check
    Method: GET /health
    ✓ PASSED (Status: 200)

========================================
User Management Tests
========================================

[2] Testing: Create User
    Method: POST /users
    ✓ PASSED (Status: 201)
      → Created User ID: U000000000000001

...

========================================
Test Results Summary
========================================
Total Tests: 20
  ✓ Passed: 15
  ✗ Failed: 0
  ⊘ Skipped: 5
Pass Rate: 75.0%

✓ All tests passed!
```

#### 옵션 B: 수동 테스트

Health endpoint 테스트:
```bash
curl https://[YOUR-API-ID].execute-api.us-east-1.amazonaws.com/v1/health
```

예상 응답:
```json
{
  "status": "ok",
  "timestamp": "2025-10-28T12:34:56Z",
  "database": "connected"
}
```

사용자 생성 테스트:
```bash
curl -X POST https://[YOUR-API-ID].execute-api.us-east-1.amazonaws.com/v1/users \
  -H "Content-Type: application/json" \
  -d '{
    "name": "홍길동",
    "email": "hong@example.com",
    "phone": "010-1234-5678"
  }'
```

차량 목록 조회:
```bash
curl https://[YOUR-API-ID].execute-api.us-east-1.amazonaws.com/v1/vehicles?status=available
```

---

## 배포 옵션

### 데이터베이스 초기화 건너뛰기
```powershell
.\Deploy-Unified-Platform.ps1 -SkipDatabase
```

### Dry Run (실제 배포 없이 확인만)
```powershell
.\Deploy-Unified-Platform.ps1 -DryRun
```

### 사용자 정의 설정 파일 사용
```powershell
.\Deploy-Unified-Platform.ps1 -ConfigFile my-config.json
```

---

## 배포된 Lambda 함수

### Node.js Lambda Functions (13개)
| 함수 이름 | 경로 | 설명 |
|-----------|------|------|
| `hypermob-health` | `lambdas/health` | 헬스 체크 |
| `hypermob-users` | `lambdas/users` | 사용자 등록 |
| `hypermob-auth-face` | `lambdas/auth-face` | 얼굴 이미지 등록 |
| `hypermob-auth-nfc` | `lambdas/auth-nfc` | NFC UID 등록 |
| `hypermob-auth-nfc-get` | `lambdas/auth-nfc-get` | NFC UID 조회 |
| `hypermob-auth-ble` | `lambdas/auth-ble` | BLE 해시키 발급 |
| `hypermob-auth-session` | `lambdas/auth-session` | 인증 세션 생성 |
| `hypermob-auth-result` | `lambdas/auth-result` | 인증 결과 보고 |
| `hypermob-auth-nfc-verify` | `lambdas/auth-nfc-verify` | NFC 최종 검증 |
| `hypermob-vehicles` | `lambdas/vehicles` | 차량 목록 조회 |
| `hypermob-bookings` | `lambdas/bookings` | 차량 예약 관리 |
| `hypermob-user-profile` | `lambdas/user-profile` | 사용자 프로필 관리 |
| `hypermob-vehicle-control` | `lambdas/vehicle-control` | 차량 설정 제어 |

### Python Lambda Functions (3개)
| 함수 이름 | 경로 | 설명 |
|-----------|------|------|
| `hypermob-facedata` | `lambdas/facedata` | 얼굴 랜드마크 추출 (MediaPipe) |
| `hypermob-measure` | `lambdas/measure` | 신체 측정 (MediaPipe Pose) |
| `hypermob-recommend` | `lambdas/recommend` | 차량 설정 추천 (ML 모델) |

---

## 데이터베이스 스키마

### 테이블 목록 (9개)

1. **users** - 사용자 기본 정보
2. **face_datas** - 얼굴 랜드마크 데이터
3. **body_datas** - 신체 측정 데이터
4. **user_profiles** - 사용자 맞춤 차량 설정
5. **cars** - 차량 정보
6. **bookings** - 차량 예약
7. **auth_sessions** - MFA 인증 세션
8. **ble_sessions** - BLE 세션
9. **vehicle_settings** - 차량 설정 기록

### 더미 데이터
- 5명의 테스트 사용자
- 8대의 차량
- 7개의 예약
- 6개의 얼굴 데이터
- 4개의 신체 측정 데이터

---

## API 엔드포인트

### 인증 관련
- `POST /users` - 사용자 등록
- `POST /auth/face` - 얼굴 이미지 등록
- `POST /auth/nfc` - NFC UID 등록
- `GET /auth/nfc` - NFC UID 조회
- `GET /auth/ble` - BLE 해시키 발급
- `GET /auth/session` - 인증 세션 시작
- `POST /auth/result` - 인증 결과 보고
- `POST /auth/nfc/verify` - NFC 최종 검증

### AI 서비스
- `POST /facedata` - 얼굴 랜드마크 추출
- `POST /measure` - 신체 측정
- `POST /recommend` - 차량 설정 추천

### 차량 및 예약
- `GET /vehicles` - 차량 목록 조회
- `POST /bookings` - 차량 예약 생성
- `GET /bookings` - 예약 목록 조회

### 사용자 프로필
- `GET /users/{user_id}/profile` - 프로필 조회
- `PUT /users/{user_id}/profile` - 프로필 생성/업데이트

### 차량 제어
- `POST /vehicles/{car_id}/settings/apply` - 차량 설정 자동 적용

### 시스템
- `GET /health` - 헬스 체크

---

## 트러블슈팅

### Lambda 함수 배포 실패
```powershell
# 개별 함수 로그 확인
aws logs tail /aws/lambda/hypermob-[function-name] --follow
```

### API Gateway 연결 실패
```bash
# Lambda 함수에 권한이 있는지 확인
aws lambda get-policy --function-name hypermob-health
```

### 데이터베이스 연결 실패
```bash
# VPC Security Group이 RDS 포트(3306)를 허용하는지 확인
# Lambda 함수가 올바른 VPC Subnet에 배포되었는지 확인
```

### Python Lambda 함수 오류
```bash
# Python 패키지 의존성이 제대로 설치되었는지 확인
cd lambdas/facedata
pip install -r requirements.txt -t .
```

---

## 환경 변수

모든 Lambda 함수는 다음 환경 변수를 사용합니다:

- `DB_HOST` - RDS 엔드포인트
- `DB_USER` - 데이터베이스 사용자 (기본값: `admin`)
- `DB_PASSWORD` - 데이터베이스 비밀번호
- `DB_NAME` - 데이터베이스 이름 (기본값: `hypermob`)

---

## 보안 고려사항

1. **VPC 격리**: Lambda 함수는 VPC 내에 배포되어 RDS에 안전하게 접근
2. **IAM 권한**: 최소 권한 원칙에 따라 필요한 권한만 부여
3. **환경 변수 암호화**: AWS Secrets Manager 사용 권장 (프로덕션 환경)
4. **API 인증**: API Gateway에 API Key 또는 JWT 인증 추가 권장

---

## 다음 단계

1. **API 문서 확인**: `docs/openapi-unified.yaml` 참조
2. **Android 앱 연동**: API 엔드포인트를 앱에 설정
3. **모니터링 설정**: CloudWatch 알람 및 대시보드 구성
4. **CI/CD 파이프라인**: GitHub Actions 또는 AWS CodePipeline 설정

---

## 지원 및 문의

- GitHub Issues: https://github.com/hypermob/platform/issues
- Email: dev@hypermob.com
