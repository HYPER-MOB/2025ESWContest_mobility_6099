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
	- 카메라로부터 얼굴 랜드마크를 캡처하고, 저장된 프로파일(`user1.txt` 같은)을 불러와 z-score 정규화 후 코사인 유사도로 인증을 시도합니다.  
	- 외부 제어 신호를 `input.txt`에서 폴링(poll)하여 동작을 전환합니다. 인증 결과 및 상태는 `output.txt`에 정수 코드로 기록합니다.

- `input.txt`  
	- 외부에서 faceauth로 보내는 제어 신호(정수). 예: 2(인증 시작), 1(대기), 0(종료).

- `output.txt`  
	- `faceauth.py`가 기록하는 상태 코드입니다. 사용된 코드(소스 기준):
		- `1` : standby(대기)
		- `2` : 인증 시작(Starting face authentication)
		- `3` : 인증 성공
		- `4` : 인증 실패
		- `0` : 프로그램 종료

- `user1.txt`  
	- 얼굴 프로파일 파일(예시). 각 라인은 인덱스(앞 세자리는 랜드마크 번호, 마지막 한자리는 좌표 축(X: 0, Y: 1, Z: 2))와 값(실수) 형식으로 되어 있고 `FFFF`로 끝납니다. `faceauth.py`는 이 파일을 파싱해 (landmark, coord) 인덱스 목록과 값 벡터를 생성합니다.

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

- 얼굴 인증 실행(SCA_Core에 의해 실행):

```cmd
python3 .\faceauth.py
```

	- `faceauth.py`는 `input.txt`를 폴링하며, 값이 `2`로 변경되면 프로파일(`user1.txt`)을 불러와 인증을 시도하고 결과를 `output.txt`에 씁니다.
	- 환경변수로 설정 가능한 옵션들:
		- `FACE_AUTH_PROFILE_PATH` (기본 `user1.txt`)
		- `FACE_AUTH_THRESHOLD` (코사인 유사도 임계값, 기본 0.99)
		- `FACE_AUTH_CAM_INDEX`, `FACE_AUTH_CAM_WIDTH`, `FACE_AUTH_CAM_HEIGHT`, `FACE_AUTH_MAX_ATTEMPTS` 등

## 입력/출력 플로우 요약

- `drowsiness.py` → `drowsiness.txt`: 졸음 감지 상태(0/1)
- 외부 시스템 → `input.txt`: 제어 신호(0/1/2)
- `faceauth.py` → `output.txt`: 인증 상태(1/2/3/4/0)

ex.) 외부 프로세스가 `input.txt`에 `2`를 쓰면 `faceauth.py`가 인증을 시작하고(`output.txt`에 `2`), 인증 성공 시 `3`, 실패 시 `4`를 씁니다.

## 주의사항

- 두 스크립트 모두 실시간 카메라(USB/노트북 내장)를 사용하므로 카메라 접근 권한이 필요합니다.  
- MediaPipe는 GPU/CPU 환경 및 플랫폼의 설치 이슈가 있을 수 있으니, 설치 로그를 확인하세요.  
- `user1.txt`의 포맷이 요구사항과 달라지면 `faceauth.py`의 `parse_profile`에서 예외가 발생합니다. 샘플 파일(`user1.txt`)을 참고하세요.