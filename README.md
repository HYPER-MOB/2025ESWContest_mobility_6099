# DCU-UI


Qt 기반 DCU(Driver Control Unit) 운영용 프론트엔드 애플리케이션입니다. 인증(Auth), 장치 상태 점검(Data Check), 메인 대시보드(Main) 화면을 포함하며, 로컬 IPC를 통해 백엔드 데몬과 통신합니다.

---

## ⚙️ Tech Stack

* **Language**: C++17
* **Framework**: Qt 5.15+
* **Build**: CMake ≥ 3.16, Ninja/Make
* **IPC**: QLocalSocket/QLocalServer(JSON 메시지)

> 기본 소켓 경로 예시: `/tmp/dcu.demo.sock`

---

## 📁 프로젝트 구조

```

project-root/

├─ CMakeLists.txt

├─ ipc_client.h / ipc_client.cpp

├─ main.cpp

├─ MainWindow.h / MainWindow.cpp / MainWindow.ui

├─ resources.qrc

├─ WarningDialog.h / WarningDialog.cpp / WarningDialog.ui

│

├─ AuthView/

│  ├─ AuthWindow.h / AuthWindow.cpp / AuthWindow.ui

│  └─ (인증 단계 프로세스)

│

├─ DataCheckView/

│  ├─ DataCheckWindow.h / DataCheckWindow.cpp / DataCheckWindow.ui

│  └─ (데이터 요청, 데이터 적용)

│

├─ MainView/

│  ├─ MainView.h / MainView.cpp / MainView.ui

│  ├─ siren.mp3

│  └─ (대시보드, 컨트롤 패널, 경보 사운드 등)

└─ assets/ (선택) 이미지, 스타일(QSS) 등 추가 리소스

```

  

---

  

## 🔍 파일/모듈 설명

  

### Top-Level

  
* **CMakeLists.txt**

  프로젝트 설정, Qt 모듈(Widgets, Network 등) 등록, 리소스(`resources.qrc`) 포함.

* **main.cpp**

  `QApplication`/`QMainWindow` . 초기 화면(Auth → DataCheck → Main) 전환 트리거 구성.

* **MainWindow.(h/cpp/ui)**

  최상위 컨테이너. 중앙에 `QStackedWidget`으로 **AuthWindow**, **DataCheckWindow**, **MainView** 페이지를 관리. 공통 단축키/메뉴, 전역 상태 라우팅, 경보 표시 엔트리 포인트.

* **resources.qrc**

  정적 리소스 등록 파일. 아이콘/이미지, `siren.mp3` 등 오디오, 선택적 스타일(QSS) 포함.

* **WarningDialog.(h/cpp)**

  시스템 경고/알림용 모달 다이얼로그. 심각도 레벨, 자동 닫힘 타이머, `siren.mp3` 재생(필요 시) 지원.

* **ipc_client.(h/cpp)**

  QLocalSocket 기반 IPC 클라이언트. JSON 메시지 송수신, 자동 재연결, `messageReceived(QJsonObject)` 시그널 제공. 예시 토픽: `auth/*`, `datacheck/*`, `main/*` 등.

### Views

* **AuthView/** → `AuthWindow.(h/cpp/ui)`

  얼굴인식/BT/NFC 등 인증 단계를 표시. IPC 수신으로 진행 상태 업데이트.

* **DataCheckView/** → `DataCheckWindow.(h/cpp/ui)`

  프로필 데이터 요청 및 적용

* **MainView/** → `MainView.(h/cpp/ui)`

  메인 대시보드와 컨트롤. 좌석/미러/핸들 등 장치 값 표시 및 사용자 상호작용. 위험 이벤트 시 `WarningDialog` 호출 및 `siren.mp3` 재생.

  

---

  

## 🧪 실행/개발 방법

  ### 💡 참고
본 프로젝트는 Qt Creator 환경에서 개발 및 빌드되었습니다.  
Qt Creator 내부에서도 `CMake` 기반 빌드가 수행되므로,
아래 명령어들은 수동 빌드 시 참고용 예시입니다.

### 1) 의존성 설치

  
* Qt 5.15+ (Widgets, Network, Multimedia 필요)
* CMake, C++17 빌드 도구(Ninja/Make)

### 2) 빌드

  

```bash

mkdir -p build && cd build

cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja ..

ninja

```

  

> Qt6 사용 시 `-DQt6_DIR` 또는 `-DCMAKE_PREFIX_PATH` 지정 필요.

  

### 3) 실행

  

```bash

./dcu_ui_frontend

```

* 기본 IPC 소켓 경로가 유효해야 백엔드와 통신합니다.

  

### 4) 리소스/사운드

  
* `resources.qrc`에 `siren.mp3`가 포함되어야 `WarningDialog`에서 재생됩니다.

---

  

## 🔌 IPC 개요(요약)


* **전송 형식**: JSON(UTF-8)

* **공통 필드 예시**:


  ```json

  { "topic": "auth/result", "ok": true, "error": 0, "reqId": 123 }

  ```

* **수신 처리 플로우**: `ipc_client` → 각 View의 `onIpcMessage(const QJsonObject&)` → UI 바인딩

---
