# ⚙️ DCU-Core Daemon

## 📘 개요

**DCU-Core**는  Driver Control Unit 시스템의 백엔드 데몬입니다.
로컬 IPC를 통해 프론트엔드와 JSON 형식으로 통신하며, 내부적으로 CAN 버스(`can0`, `can1`)를 제어하여 실제 하드웨어 데이터를 송수신합니다.

---

## ⚙️ Tech Stack

* **Language**: C++17
* **Build System**: CMake ≥ 3.16
* **CAN Library**: Library-CAN (외부 의존성)
* **IPC**: QLocalServer / QLocalSocket(JSON 기반)
* **Target OS**: Linux (예: Raspberry Pi Ubuntu 22.04)

>  `librarycan`은 반드시 프로그램 폴더에 포함 되어 있어야 합니다. 

---

## 📁 프로젝트 구조

```
dcu-core/
├─ CMakeLists.txt
├─ ipc.h
├─ ipc.cpp
└─ main.cpp
```

### 📂 파일 설명

* **CMakeLists.txt**
  프로젝트 설정, `librarycan` 링크, IPC 소켓 권한 설정 등 포함.

* **ipc.h / ipc.cpp**
  `QLocalServer` 기반 IPC 서버 모듈.
  클라이언트 연결 관리, 메시지 수신 파싱, 브로드캐스트 처리 수행.

* **main.cpp**
  데몬 엔트리 포인트.

  * CAN 초기화 (`can_init()`)
  * IPC 서버 실행(`/tmp/dcu.demo.sock`)
  * 주기적 CAN 수신 처리 및 상태 갱신
  * IPC를 통한 UI로 데이터 전송

---

## 🧱 의존성

| 항목                 | 설명                                |
| ------------------ | --------------------------------- |
| QtCore / QtNetwork | IPC 통신 구현에 필요                     |
| Library-CAN         | CAN 통신용 C/C++ 라이브러리  |

---

## 🧪 빌드 방법

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```
---

## 🚀 실행 절차

   ```bash
   sudo ./ipc_demo
   ```
IPC 서버가 `/tmp/dcu.demo.sock` 경로에서 클라이언트 대기.

---

## 🔌 IPC 메시지 포맷

### 송신 예시 (CAN → UI)

```json
{
  "topic": "seat/state",
  "pos": 45,
  "angle": 10,
  "status": 1
}
```

### 수신 예시 (UI → Core)

```json
{
  "topic": "mirror/order",
  "yaw": -20,
  "pitch": 5,
  "apply": true
}
```

---

## 🧩 주요 동작 흐름

1. **CAN 초기화** → `librarycan`을 통해 `can0`, `can1` 활성화.
2. **IPC 서버 시작** → `/tmp/dcu.demo.sock` 생성 후 UI 연결 대기.
3. **CAN 프레임 수신** → 20ms 주기로 상태 업데이트.
4. **상태 브로드캐스트** → JSON으로 UI에 전송.
5. **명령 수신 처리** → UI에서 온 order 메시지를 분석 후 CAN 송신.

---

