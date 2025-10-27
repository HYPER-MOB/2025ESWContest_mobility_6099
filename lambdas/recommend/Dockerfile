FROM public.ecr.aws/lambda/python:3.9

# Python 의존성 설치
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# 훈련 데이터 및 함수 코드 복사
COPY lambda_handler.py ${LAMBDA_TASK_ROOT}/
COPY json_template.json ${LAMBDA_TASK_ROOT}/

# 환경 변수 설정
ENV AWS_REGION=ap-northeast-2
ENV MODEL_BUCKET=hypermob-models
ENV MODEL_KEY=car_fit_model.joblib

# Lambda 핸들러 지정
CMD ["lambda_handler.handler"]
