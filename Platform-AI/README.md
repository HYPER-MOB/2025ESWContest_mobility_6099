# HYPERMOB Platform AI

신체 측정 및 차량 설정 추천을 위한 AI 서비스

## 📋 개요

이 프로젝트는 MediaPipe를 활용한 신체/얼굴 측정과 머신러닝 기반 차량 설정 추천 시스템을 AWS Lambda로 구현한 서버리스 API입니다.

### 주요 기능

1. **신체 측정 (Measure)**: MediaPipe Pose로 전신 이미지에서 팔/다리/몸통 길이 측정
2. **차량 설정 추천 (Recommend)**: Ridge 회귀 모델로 신체 치수에 맞는 시트/미러/핸들 설정 추천
3. **얼굴 랜드마크 (Facedata)**: MediaPipe FaceMesh로 468개 얼굴 특징점 추출

## 🏗️ 프로젝트 구조

```
Platform-AI/
├── docs/                           # 📄 문서
│   ├── guides/                     # 가이드 문서
│   ├── improvements/               # 기능 개선 문서
│   └── archived/                   # 과거 문서
├── scripts/                        # 🔧 스크립트
│   ├── deploy/                     # 배포 스크립트
│   ├── test/                       # 테스트 스크립트
│   └── utils/                      # 유틸리티 스크립트
├── src/                            # 💻 원본 소스 코드
├── lambdas/                        # ☁️ Lambda 함수
│   ├── measure/
│   ├── recommend/
│   └── facedata/
├── openapi.yml                     # OpenAPI 3.0 스펙
└── README.md                       # 이 파일
```

## 🚀 빠른 시작

### 전제 조건
- AWS CLI 설정 완료
- Docker Desktop 실행 중
- PowerShell 5.1 이상

### 1. 전체 배포
```powershell
.\scripts\deploy\deploy.ps1
```

### 2. 개별 Lambda 재배포
```powershell
.\scripts\deploy\rebuild_measure.ps1     # Measure만
.\scripts\deploy\rebuild_recommend.ps1   # Recommend만
.\scripts\deploy\rebuild_facedata.ps1    # Facedata만
```

### 3. 테스트
```powershell
.\scripts\test\test_all_endpoints.ps1
```

## 📡 API 엔드포인트

**Base URL:** `https://{api-id}.execute-api.ap-northeast-2.amazonaws.com/prod`

- `GET /health` - Health Check
- `POST /measure` - 신체 측정
- `POST /recommend` - 차량 설정 추천
- `POST /facedata` - 얼굴 랜드마크 추출

## 📚 문서

- [빠른 시작 가이드](docs/guides/QUICKSTART.md)
- [배포 가이드](docs/guides/DEPLOYMENT.md)
- [테스트 시나리오](docs/guides/TEST_SCENARIOS.md)

## 🛠️ 기술 스택

- **AWS Lambda**: 서버리스 컴퓨팅
- **API Gateway**: REST API
- **MediaPipe**: Pose & FaceMesh
- **scikit-learn**: ML 모델
- **Docker**: 컨테이너화

## 📞 문의

Platform AI Team - HYPERMOB

---

**Version**: 1.0.0  
**Last Updated**: 2025-10-27  
**Status**: ✅ Production Ready

## 📋 프로젝트 구조 변경

최근 프로젝트 구조가 개선되었습니다. 기존 명령어가 작동하지 않는다면 [마이그레이션 가이드](docs/MIGRATION_GUIDE.md)를 참고하세요.

