# SCA-AI

## 개요

이 저장소는 카메라 기반의 졸음(눈감음/머리 기울기/하품) 감지 모듈과 얼굴 인증(프로파일 매칭) 모듈의 간단한 데모/유틸리티 코드로 구성되어 있습니다. 주로 OpenCV, MediaPipe, NumPy 를 사용합니다.

## 파일 설명

- `drowsiness.py`  
    - 실시간 웹캠으로부터 얼굴 랜드마크를 얻어 EYE ASPECT RATIO(EAR), MOUTH ASPECT RATIO(MAR), 그리고 머리 기울기(roll/pitch)를 계산하여 졸음 상태를 판단합니다.  
    - 결과(현재 상태: 1=drowsy, 0=ok)는 `drowsiness.txt` 파일에 주기적으로 기록됩니다. 프로그램 시작 시 `drowsiness.txt`에 `2`를 써서 초기화하는 부분이 있습니다.

- `drowsiness.txt`  
    - `drowsiness.py`가 상태를 쓰는 간단한 텍스트 파일(예: `0` 또는 `1`).

- `faceauth.py`  
    - 카메라로부터 얼굴 랜드마크를 캡처하고, 저장된 프로파일(ex. `user1.txt`)을 불러와 "XY 기반 거리 비율" 특성 벡터를 만든 뒤(z-score 정규화) 코사인 유사도로 인증을 시도합니다.  
        - 좌·우 눈 외측(landmark 33, 263)이 있으면 두 점 사이 거리(IPD)로 정규화, 없으면 얼굴 랜드마크 중심(centroid) 기반 정규화 사용  
        - 시도당 여러 프레임을 수집해 평균 벡터를 사용하여 노이즈를 감소시킵니다.  
        - 인증 전 liveness 검사 수행: 입 벌림(MAR: 13–14/78–308) 임계 초과 또는 연속 프레임 간 정규화된 미세 움직임이 임계 초과 시 live로 판정. 실패 시 인증을 진행하지 않고 `output.txt`에 `4` 기록
    - 외부 제어 신호를 `input.txt`에서 폴링(poll)하여 동작을 전환합니다. 인증 결과 및 상태는 `output.txt`에 정수 코드로 기록합니다.

- `input.txt`  
    - 외부에서 faceauth로 보내는 제어 신호:
		- `1` : 대기
		- `2` : 인증 시작
		- `0` : 종료

- `output.txt`  
    - `faceauth.py`가 기록하는 상태 코드:
        - `1` : standby(대기)
        - `2` : 인증 시작(Starting face authentication)
        - `3` : 인증 성공
        - `4` : 인증 실패
        - `0` : 프로그램 종료

- `user1.txt`  
    - 얼굴 프로파일 파일(예시). 각 라인은 인덱스(앞 세자리는 랜드마크 번호, 마지막 한자리는 좌표 축(X: 0, Y: 1, Z: 2))와 값(실수) 형식으로 되어 있고 `FFFF`로 끝납니다. `faceauth.py`는 이 파일을 파싱해 (landmark, coord) 인덱스 목록과 값 벡터를 생성합니다.  
    - 비율 기반 비교는 X/Y 좌표만 사용합니다(Z=2는 무시). 가능하면 동일 랜드마크의 X와 Y가 함께 포함되도록 구성하세요.  
    - 랜드마크 33과 263(좌/우 눈 외측)이 포함되어 있으면 IPD 정규화가 적용됩니다(없으면 centroid 정규화).

- `input.txt`, `output.txt`, `drowsiness.txt`, `user1.txt`는 텍스트 기반의 I/O 인터페이스로 서로 간단한 IPC 역할을 합니다(동일 디렉터리에서 파일 읽기/쓰기).

## 의존성

- Miniconda 25.3.1
- Python 3.12.11
- opencv-python 4.11.0.86
- mediapipe 0.10.18
- numpy 1.26.4

※ 운영체제, Python 버전, MediaPipe 버전 간 호환성 문제에 유의하세요. MediaPipe는 특정 플랫폼/버전에서 설치 이슈가 있을 수 있습니다.

## 실행 방법

- 졸음 감지 실행:

```cmd
python3 .\drowsiness.py
```

    - 웹캠을 사용합니다. 실행 중 `drowsiness.txt`에 `1`(졸음) 또는 `0`(정상)이 주기적으로 기록됩니다.

- 얼굴 인증 실행(단독 실행 또는 SCA_Core에서 호출):

```cmd
python3 .\faceauth.py
```

    - `faceauth.py`는 `input.txt`를 폴링하며, 값이 `2`로 변경되면 프로파일(`user1.txt`)을 불러와 인증을 시도하고 결과를 `output.txt`에 씁니다.
    - 환경변수로 설정 가능한 옵션들(소스 기준):
        - `FACE_AUTH_PROFILE_PATH` (기본 `user1.txt`)
        - `FACE_AUTH_INPUT_FILE` (기본 `input.txt`), `FACE_AUTH_OUTPUT_FILE` (기본 `output.txt`)
        - `FACE_AUTH_THRESHOLD` (코사인 유사도 임계값, 기본 `0.99`)
        - `FACE_AUTH_POLL_SEC` (폴링 주기 초, 기본 `0.1`)
        - `FACE_AUTH_FRAMES_PER_ATTEMPT` (시도당 취득 프레임 수, 평균 벡터 계산용, 기본 `5`)
        - 카메라 관련: `FACE_AUTH_CAM_INDEX`(기본 `0`), `FACE_AUTH_CAM_WIDTH`(기본 `1280`), `FACE_AUTH_CAM_HEIGHT`(기본 `720`), `FACE_AUTH_CAM_FPS`(기본 `30`)
        - MediaPipe 관련: `FACE_AUTH_REFINE_LANDMARKS`(기본 `1`), `FACE_AUTH_DET_CONF`(기본 `0.9`), `FACE_AUTH_TRK_CONF`(기본 `0.9`), `FACE_AUTH_MAX_FACES`(기본 `1`)
        - Liveness 관련:
            - `FACE_AUTH_LIVENESS` (0/1, 기본 `1`)
            - `FACE_AUTH_LIVE_MAX_FRAMES` (검사 프레임 상한, 기본 `80`)
            - `FACE_AUTH_LIVE_TIMEOUT_S` (검사 타임아웃 초, 기본 `6.0`)
            - `FACE_AUTH_LIVE_MAR_THRESH` (입 벌림 임계값, 기본 `0.22`)
            - `FACE_AUTH_LIVE_MOTION_THRESH` (프레임 간 평균 정규화 이동 임계값, 기본 `0.003`)

## 입력/출력 플로우 요약

- `drowsiness.py` → `drowsiness.txt`: 졸음 감지 상태(0/1)
- 외부 시스템 → `input.txt`: 제어 신호(0/1/2)
- `faceauth.py` → `output.txt`: 인증 상태(1/2/3/4/0)

- `faceauth.py` 시작 시 `output.txt`에 `1`(standby)을 먼저 기록합니다.
- `drowsiness.py`는 시작 시 `drowsiness.txt`에 `2`를 기록하여 초기화합니다.
- `4`는 liveness 실패를 포함한 인증 실패 코드입니다.

ex.) 외부 프로세스가 `input.txt`에 `2`를 쓰면 `faceauth.py`가 liveness 검사를 먼저 수행하고, 통과 시 프로파일 매칭을 진행합니다. 통과하지 못하면 즉시 `4`를 씁니다. 매칭 성공 시 `3`, 실패 시 `4`를 씁니다.



## 주의사항

- 두 스크립트 모두 실시간 카메라(USB/노트북 내장)를 사용하므로 카메라 접근 권한이 필요합니다.  
- MediaPipe는 GPU/CPU 환경 및 플랫폼의 설치 이슈가 있을 수 있으니, 설치 로그를 확인하세요.  
- `user1.txt`의 포맷이 요구사항과 달라지면 `faceauth.py`의 `parse_profile`에서 예외가 발생합니다. 샘플 파일(`user1.txt`)을 참고하세요.