#!/usr/bin/env python3
"""
MediaPipe FaceMesh 모델 사전 다운로드 스크립트
Docker 빌드 시 실행되어 모델 파일을 미리 다운로드합니다.
"""

import os
import sys

# MediaPipe 환경 변수 설정
os.environ['MEDIAPIPE_DISABLE_GPU'] = '1'
os.environ['GLOG_logtostderr'] = '1'

print("Downloading MediaPipe FaceMesh models...")

try:
    import mediapipe as mp

    print("Initializing FaceMesh model (refine=False)...")
    mp_face_mesh = mp.solutions.face_mesh

    with mp_face_mesh.FaceMesh(
        static_image_mode=True,
        refine_landmarks=False,
        max_num_faces=1,
        min_detection_confidence=0.5
    ) as face_mesh:
        print("FaceMesh model (refine=False) downloaded successfully!")

    print("Initializing FaceMesh model (refine=True)...")
    with mp_face_mesh.FaceMesh(
        static_image_mode=True,
        refine_landmarks=True,
        max_num_faces=1,
        min_detection_confidence=0.5
    ) as face_mesh:
        print("FaceMesh model (refine=True) downloaded successfully!")

    print("All FaceMesh models downloaded!")
    sys.exit(0)

except Exception as e:
    print(f"Error downloading models: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)
