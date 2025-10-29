FROM public.ecr.aws/lambda/python:3.9

# 시스템 의존성 설치 (OpenCV, MediaPipe 필요)
RUN yum install -y \
    mesa-libGL \
    libgomp \
    glib2 \
    && yum clean all

# Python 의존성 설치
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# MediaPipe 모델 파일 사전 다운로드 스크립트 복사 및 실행
COPY download_models.py /tmp/download_models.py
RUN python3.9 /tmp/download_models.py && rm /tmp/download_models.py

# Lambda 함수 코드 복사
COPY lambda_handler.py ${LAMBDA_TASK_ROOT}/

# 환경 변수 설정
ENV RESULT_BUCKET=hypermob-results
ENV AWS_REGION=ap-northeast-2
ENV MEDIAPIPE_DISABLE_GPU=1
ENV GLOG_logtostderr=1

# Lambda 핸들러 지정
CMD ["lambda_handler.handler"]
