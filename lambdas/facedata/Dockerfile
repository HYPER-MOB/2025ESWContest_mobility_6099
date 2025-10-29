FROM public.ecr.aws/lambda/python:3.9

# 시스템 의존성 설치 (OpenCV, MediaPipe 필요)
RUN yum install -y \
    mesa-libGL \
    libXrender \
    libXext \
    libSM \
    libICE \
    && yum clean all \
    && rm -rf /var/cache/yum

# Python 의존성 설치
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Lambda 함수 코드 복사
COPY lambda_handler.py ${LAMBDA_TASK_ROOT}/

# 환경 변수 설정
ENV AWS_REGION=ap-northeast-2
ENV RESULT_BUCKET=hypermob-results
ENV MEDIAPIPE_DISABLE_GPU=1
ENV GLOG_logtostderr=1
ENV TMPDIR=/tmp

# Lambda 핸들러 지정
CMD ["lambda_handler.handler"]
