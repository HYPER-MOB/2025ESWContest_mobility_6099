# Docker 설정 가이드

## 문제 진단

다음 에러가 발생하는 경우:
```
ERROR: error during connect: Head "http://%2F%2F.%2Fpipe%2FdockerDesktopLinuxEngine/_ping":
open //./pipe/dockerDesktopLinuxEngine: The system cannot find the file specified.
```

**원인**: Docker Desktop이 실행되지 않았거나 설치되지 않음

## 해결 방법

### 1. Docker Desktop 설치 확인

#### Windows에서 확인

```powershell
# PowerShell에서 실행
docker --version
```

**설치되지 않은 경우**:
1. [Docker Desktop for Windows](https://www.docker.com/products/docker-desktop) 다운로드
2. 설치 파일 실행
3. WSL 2 설치 옵션 선택 (권장)
4. 컴퓨터 재시작

### 2. Docker Desktop 시작

#### 방법 1: GUI로 시작
1. Windows 시작 메뉴에서 "Docker Desktop" 검색
2. Docker Desktop 실행
3. Docker 아이콘이 시스템 트레이에 나타날 때까지 대기 (1-2분)
4. 아이콘이 초록색으로 변하면 준비 완료

#### 방법 2: PowerShell로 시작

```powershell
# Docker Desktop 시작
Start-Process "C:\Program Files\Docker\Docker\Docker Desktop.exe"

# 30초 대기
Start-Sleep -Seconds 30

# Docker 상태 확인
docker ps
```

### 3. Docker 상태 확인

```bash
# Docker 데몬 확인
docker ps

# Docker 정보 확인
docker info

# Docker 버전 확인
docker version
```

**성공 시 출력 예시**:
```
CONTAINER ID   IMAGE     COMMAND   CREATED   STATUS    PORTS     NAMES
```

### 4. WSL 2 설정 (Windows)

Docker Desktop은 WSL 2를 사용합니다.

#### WSL 2 설치 확인

```powershell
# PowerShell 관리자 권한으로 실행
wsl --list --verbose
```

#### WSL 2 미설치 시

```powershell
# 1. WSL 설치
wsl --install

# 2. 컴퓨터 재시작

# 3. WSL 2를 기본으로 설정
wsl --set-default-version 2
```

### 5. Docker Desktop 설정 확인

Docker Desktop 실행 후:

1. **Settings (톱니바퀴 아이콘)** 클릭
2. **General** 탭:
   - ✅ Use the WSL 2 based engine (권장)
   - ✅ Start Docker Desktop when you log in (선택사항)

3. **Resources** 탭:
   - **Memory**: 최소 4GB (8GB 권장)
   - **CPUs**: 최소 2개 (4개 권장)
   - **Disk image size**: 최소 64GB

4. **Apply & Restart** 클릭

## 배포 재시도

Docker Desktop이 실행된 후:

### PowerShell

```powershell
# Docker 상태 확인
docker ps

# 확인되면 배포 재시도
.\deploy.ps1
```

### Git Bash

```bash
# Docker 상태 확인
docker ps

# 확인되면 배포 재시도
./deploy.sh
```

## 자주 발생하는 문제

### 문제 1: WSL 2 설치 에러

**증상**:
```
WslRegisterDistribution failed with error: 0x800701bc
```

**해결**:
1. [WSL 2 Linux 커널 업데이트 패키지](https://aka.ms/wsl2kernel) 다운로드
2. 설치 후 재시작

### 문제 2: Docker Desktop이 시작되지 않음

**해결**:
1. 작업 관리자에서 모든 Docker 프로세스 종료
2. Docker Desktop 재시작
3. 그래도 안 되면 Docker Desktop 재설치

### 문제 3: Hyper-V 충돌

**증상**:
```
Hardware assisted virtualization and data execution protection must be enabled in the BIOS
```

**해결**:
```powershell
# PowerShell 관리자 권한으로 실행
# Hyper-V 활성화
Enable-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V -All

# 재시작
Restart-Computer
```

### 문제 4: 방화벽/바이러스 백신 충돌

Docker Desktop이 방화벽/백신 프로그램에 의해 차단될 수 있습니다.

**해결**:
1. Docker Desktop을 방화벽 예외에 추가
2. 백신 프로그램 실시간 검사에서 Docker 폴더 제외:
   - `C:\Program Files\Docker\
   - `C:\ProgramData\Docker\

## Docker 없이 배포하기 (대안)

Docker 설치가 어려운 경우, AWS CloudShell 사용:

### 1. AWS Console에서 CloudShell 열기

1. [AWS Console](https://console.aws.amazon.com) 로그인
2. 우측 상단 CloudShell 아이콘 클릭
3. 프로젝트 파일 업로드

### 2. CloudShell에서 배포

```bash
# 파일 업로드 후
cd Platform-AI
chmod +x deploy.sh
./deploy.sh
```

## Docker 설치 없이 Lambda 배포 (ZIP 방식)

measure 함수는 대용량 라이브러리로 인해 ZIP 방식이 어렵지만,
recommend 함수는 ZIP으로 배포 가능:

```bash
# recommend 함수만 ZIP으로 배포
cd recommend
mkdir package
pip install -r requirements.txt -t package/
cp lambda_handler.py package/
cp json_template.json package/
cd package
zip -r ../recommend-function.zip .
cd ..

# Lambda 함수 생성
aws lambda create-function \
  --function-name hypermob-recommend \
  --runtime python3.9 \
  --role arn:aws:iam::ACCOUNT_ID:role/HypermobLambdaExecutionRole \
  --handler lambda_handler.handler \
  --zip-file fileb://recommend-function.zip \
  --timeout 60 \
  --memory-size 1024
```

## 확인 체크리스트

배포 전 확인:

- [ ] Docker Desktop 설치됨
- [ ] Docker Desktop 실행 중 (시스템 트레이 초록색 아이콘)
- [ ] `docker ps` 명령어 작동
- [ ] WSL 2 설치됨 (Windows)
- [ ] 메모리 최소 4GB 할당
- [ ] AWS CLI 설정 완료 (`aws configure`)
- [ ] 인터넷 연결 안정적

## 도움이 필요한 경우

1. Docker Desktop 로그 확인:
   - Docker Desktop → Troubleshoot → View logs

2. WSL 로그 확인:
   ```powershell
   wsl --list --verbose
   ```

3. Docker 재설치:
   - Docker Desktop 완전 삭제
   - 재시작
   - 최신 버전 재설치
