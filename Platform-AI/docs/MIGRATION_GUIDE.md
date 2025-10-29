# 프로젝트 구조 변경 가이드

## 📦 변경 사항 요약

프로젝트가 더 체계적으로 정리되었습니다!

### 폴더 구조 변경

| 이전 | 이후 | 설명 |
|------|------|------|
| `./measure/` | `./lambdas/measure/` | Lambda 함수 통합 |
| `./recommend/` | `./lambdas/recommend/` | Lambda 함수 통합 |
| `./facedata/` | `./lambdas/facedata/` | Lambda 함수 통합 |
| `./measure.py` | `./src/measure.py` | 원본 소스 |
| `./recommend.py` | `./src/recommend.py` | 원본 소스 |
| `./facedata.py` | `./src/facedata.py` | 원본 소스 |
| `./DEPLOYMENT.md` | `./docs/guides/DEPLOYMENT.md` | 문서 분류 |
| `./deploy.ps1` | `./scripts/deploy/deploy.ps1` | 스크립트 분류 |
| `./test_all_endpoints.ps1` | `./scripts/test/test_all_endpoints.ps1` | 테스트 분류 |

### 스크립트 경로 업데이트

**이전 명령어:**
```powershell
.\rebuild_measure.ps1
.\test_all_endpoints.ps1
```

**새 명령어:**
```powershell
.\scripts\deploy\rebuild_measure.ps1
.\scripts\test\test_all_endpoints.ps1
```

### 제거된 파일

- ❌ `*.sh` (Bash 스크립트 - PowerShell로 통일)
- ❌ `deploy_simple.ps1` (중복)
- ❌ `redeploy_functions.ps1` (rebuild_* 스크립트로 대체)
- ❌ `temp/` 폴더

## 🔄 마이그레이션 단계

### 1. Git Pull (이미 푸시된 경우)

```bash
git pull origin main
```

### 2. 로컬 변경사항 확인

```bash
# 변경되지 않은 파일 확인
git status
```

### 3. 새 구조로 명령어 업데이트

#### 배포
```powershell
# 이전
.\rebuild_measure.ps1

# 이후
.\scripts\deploy\rebuild_measure.ps1
```

#### 테스트
```powershell
# 이전
.\test_all_endpoints.ps1

# 이후
.\scripts\test\test_all_endpoints.ps1
```

### 4. IDE/편집기 설정 업데이트

VSCode `tasks.json` 예시:
```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Rebuild Measure",
      "type": "shell",
      "command": ".\\scripts\\deploy\\rebuild_measure.ps1",
      "problemMatcher": []
    },
    {
      "label": "Test All",
      "type": "shell",
      "command": ".\\scripts\\test\\test_all_endpoints.ps1",
      "problemMatcher": []
    }
  ]
}
```

## 📁 새 폴더 구조 이점

### 1. 명확한 분리
- **소스 코드**: `src/` (원본 Python)
- **Lambda 함수**: `lambdas/` (AWS 배포용)
- **문서**: `docs/` (가이드, 개선사항)
- **스크립트**: `scripts/` (배포, 테스트, 유틸)

### 2. 확장성
```
scripts/
├── deploy/       # 배포 관련 (새 Lambda 추가 시 여기에)
├── test/         # 테스트 관련 (새 테스트 추가 시 여기에)
└── utils/        # 유틸리티 (기타 스크립트)
```

### 3. 문서 분류
```
docs/
├── guides/       # 사용자 가이드
├── improvements/ # 기술 문서
└── archived/     # 과거 문서 (참고용)
```

## 🔍 자주 묻는 질문

### Q1: 기존 스크립트가 작동하지 않아요

**A:** 경로가 변경되었습니다. 새 경로를 사용하세요:
```powershell
# 예전: .\rebuild_measure.ps1
# 지금: .\scripts\deploy\rebuild_measure.ps1
```

### Q2: Lambda 함수 코드는 어디에 있나요?

**A:** `lambdas/` 폴더에 있습니다:
- `lambdas/measure/lambda_handler.py
- `lambdas/recommend/lambda_handler.py
- `lambdas/facedata/lambda_handler.py

### Q3: 원본 Python 파일은요?

**A:** `src/` 폴더에 있습니다:
- `src/measure.py
- `src/recommend.py
- `src/facedata.py

### Q4: 문서는 어디에 있나요?

**A:** `docs/` 폴더에 분류되어 있습니다:
- 가이드: `docs/guides/
- 기술 문서: `docs/improvements/
- 과거 문서: `docs/archived/

### Q5: Bash 스크립트는 어디 갔나요?

**A:** PowerShell로 통일했습니다. Windows 환경에서 더 안정적입니다.

## ✅ 확인 사항

마이그레이션 후 확인:

- [ ] `.\scripts\deploy\rebuild_measure.ps1` 실행 가능
- [ ] `.\scripts\test\test_all_endpoints.ps1` 실행 가능
- [ ] API 정상 작동 (Health Check)
- [ ] 문서 접근 가능 (`docs/` 폴더)

## 📞 문제 발생 시

1. **경로 오류**: 프로젝트 루트에서 실행하는지 확인
2. **스크립트 실패**: `.\scripts\deploy\` 경로 확인
3. **Lambda 오류**: CloudWatch Logs 확인

---

**적용 날짜**: 2025-10-27
**버전**: 1.0.0 → 1.1.0 (구조 개선)
