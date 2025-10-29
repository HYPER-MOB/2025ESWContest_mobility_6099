#!/usr/bin/env python3
"""
MediaPipe 모델 파일 사전 다운로드 스크립트
Docker 빌드 시 실행되어 모델 파일을 미리 다운로드합니다.
"""

import os
import sys

# 환경 변수 설정
os.environ['MEDIAPIPE_DISABLE_GPU'] = '1'
os.environ['GLOG_logtostderr'] = '1'

print("Downloading MediaPipe models...")

try:
    import mediapipe as mp

    # Pose 모델 초기화 (모델 파일 자동 다운로드)
    print("Initializing Pose model (complexity 2)...")
    mp_pose = mp.solutions.pose

    # 모델 다운로드를 위해 Pose 객체 생성
    with mp_pose.Pose(
        static_image_mode=True,
        model_complexity=2,
        enable_segmentation=False
    ) as pose:
        print("Pose model downloaded successfully!")

    print("All models downloaded!")
    sys.exit(0)

except Exception as e:
    print(f"Error downloading models: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)
