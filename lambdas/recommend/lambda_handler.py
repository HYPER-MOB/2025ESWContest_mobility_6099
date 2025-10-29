"""
Lambda Handler for Car Setup Recommendation Service
차량 설정 추천 Lambda 함수
"""

import json
import os
import traceback
import numpy as np
import pandas as pd
import joblib
import boto3
from sklearn.pipeline import Pipeline
from sklearn.preprocessing import StandardScaler
from sklearn.linear_model import RidgeCV, Ridge
from sklearn.model_selection import KFold

# 전역 변수 (Lambda 컨테이너 재사용 최적화)
model = None
feature_cols = None

# 타겟 값 범위
TARGET_RANGES = {
    "seat_position": (0, 100),        # percent
    "seat_angle": (0, 180),          # deg
    "seat_front_height": (0, 100),    # percent
    "seat_rear_height": (0, 100),     # percent
    "mirror_left_yaw": (0, 180),     # deg
    "mirror_left_pitch": (0, 180),   # deg
    "mirror_right_yaw": (0, 180),    # deg
    "mirror_right_pitch": (0, 180),  # deg
    "mirror_room_yaw": (0, 180),     # deg
    "mirror_room_pitch": (0, 180),   # deg
    "wheel_position": (0, 100),      # percent
    "wheel_angle": (0, 180)           # deg
}


def load_training_data(json_path):
    """훈련 데이터 로드"""
    with open(json_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    df = pd.DataFrame(data)

    feature_cols = ["height", "upper_arm", "forearm", "thigh", "calf", "torso"]
    target_cols = [
        "seat_position", "seat_angle", "seat_front_height", "seat_rear_height",
        "mirror_left_yaw", "mirror_left_pitch", "mirror_right_yaw", "mirror_right_pitch",
        "mirror_room_yaw", "mirror_room_pitch", "wheel_position", "wheel_angle"
    ]

    X = df[feature_cols].values
    y = df[target_cols].values
    return X, y, feature_cols, target_cols


def train_model(X, y):
    """Ridge 회귀 모델 훈련"""
    alphas = np.logspace(-3, 3, 13)

    n_samples = len(X)

    if n_samples < 3:
        print("Warning: Very few samples, using default Ridge")
        ridge = Ridge(alpha=1.0)
        pipe = Pipeline([
            ("scaler", StandardScaler()),
            ("ridge", ridge)
        ])
        pipe.fit(X, y)
        return pipe

    # Cross-validation
    cv_splits = min(5, n_samples)
    kf = KFold(n_splits=cv_splits, shuffle=True, random_state=42)
    ridge = RidgeCV(alphas=alphas, cv=kf, scoring='neg_mean_squared_error')

    pipe = Pipeline([
        ("scaler", StandardScaler()),
        ("ridge", ridge)
    ])

    pipe.fit(X, y)
    print(f"Model trained with alpha: {pipe['ridge'].alpha_}")
    return pipe


def predict_car_setup(model, user_measure, feature_cols):
    """차량 설정 예측"""
    # 입력 데이터 준비
    x_new = np.array([[user_measure[c] for c in feature_cols]])

    # 예측 수행
    raw_pred = model.predict(x_new)[0]

    # 타겟 순서
    target_order = [
        "seat_position", "seat_angle", "seat_front_height", "seat_rear_height",
        "mirror_left_yaw", "mirror_left_pitch", "mirror_right_yaw", "mirror_right_pitch",
        "mirror_room_yaw", "mirror_room_pitch", "wheel_position", "wheel_angle"
    ]

    # 값 클리핑 및 정수 변환
    result = {}
    for i, key in enumerate(target_order):
        low, high = TARGET_RANGES[key]
        clipped = np.clip(raw_pred[i], low, high)
        result[key] = int(round(clipped))

    return result


def download_model_from_s3():
    """S3에서 모델 다운로드"""
    s3_bucket = os.environ.get('MODEL_BUCKET', 'hypermob-models')
    s3_key = os.environ.get('MODEL_KEY', 'car_fit_model.joblib')
    local_path = '/tmp/car_fit_model.joblib'

    # 이미 다운로드되어 있으면 재사용 (Lambda 컨테이너 재사용 최적화)
    if os.path.exists(local_path):
        print(f"Model already exists at {local_path}")
        return local_path

    try:
        print(f"Downloading model from s3://{s3_bucket}/{s3_key} to {local_path}")
        s3_client = boto3.client('s3', region_name=os.environ.get('AWS_REGION', 'ap-northeast-2'))
        s3_client.download_file(s3_bucket, s3_key, local_path)
        print(f"Model downloaded successfully: {os.path.getsize(local_path)} bytes")
        return local_path
    except Exception as e:
        print(f"Failed to download model from S3: {str(e)}")
        print(traceback.format_exc())
        raise


def initialize_model():
    """모델 초기화 (Lambda 컨테이너 시작 시 한 번만 실행)"""
    global model, feature_cols

    if model is not None:
        print("Model already loaded")
        return

    try:
        # S3에서 사전 학습된 모델 다운로드 및 로드
        model_path = download_model_from_s3()
        print(f"Loading pre-trained model from {model_path}")
        model = joblib.load(model_path)
        feature_cols = ["height", "upper_arm", "forearm", "thigh", "calf", "torso"]
        print("Model initialization complete")

    except Exception as e:
        print(f"Failed to load model from S3: {str(e)}")
        print("Falling back to training new model from template data")

        try:
            # S3 모델 로드 실패 시 훈련 데이터로 새로 학습 (fallback)
            data_path = "/var/task/json_template.json"
            if not os.path.exists(data_path):
                raise RuntimeError(f"Training data not found at {data_path}")

            print("Training new model from json_template.json")
            X, y, feature_cols, target_cols = load_training_data(data_path)
            model = train_model(X, y)

            # 모델 저장 (다음 콜드 스타트 대비)
            try:
                joblib.dump(model, "/tmp/car_fit_model.joblib")
                print("Model saved to /tmp/car_fit_model.joblib")
            except Exception as save_e:
                print(f"Warning: Could not save model: {save_e}")

            print("Model initialization complete (trained from template)")

        except Exception as fallback_e:
            print(f"Model initialization error: {str(fallback_e)}")
            print(traceback.format_exc())
            raise


# Lambda 컨테이너 시작 시 모델 로드
initialize_model()


def handler(event, context):
    """Lambda 핸들러"""
    print(f"Event: {json.dumps(event)}")

    try:
        # 모델 확인
        if model is None:
            raise RuntimeError("Model not initialized")

        # API Gateway 프록시 통합 처리
        if 'body' in event:
            if isinstance(event['body'], str):
                body = json.loads(event['body'])
            else:
                body = event['body']
        else:
            body = event

        # 입력 검증
        required_fields = ["height", "upper_arm", "forearm", "thigh", "calf", "torso"]
        missing_fields = [field for field in required_fields if field not in body]

        if missing_fields:
            return {
                'statusCode': 400,
                'headers': {
                    'Content-Type': 'application/json',
                    'Access-Control-Allow-Origin': '*'
                },
                'body': json.dumps({
                    'status': 'error',
                    'error': {
                        'code': 'MISSING_FIELDS',
                        'message': f'Missing required fields: {", ".join(missing_fields)}'
                    }
                })
            }

        # 입력 데이터 파싱 및 검증
        user_measure = {}
        for field in required_fields:
            try:
                value = float(body[field])
                if value <= 0 or value > 300:  # 신체 치수 범위 검증
                    raise ValueError(f"{field} must be between 0 and 300")
                user_measure[field] = value
            except (ValueError, TypeError) as e:
                return {
                    'statusCode': 400,
                    'headers': {
                        'Content-Type': 'application/json',
                        'Access-Control-Allow-Origin': '*'
                    },
                    'body': json.dumps({
                        'status': 'error',
                        'error': {
                            'code': 'INVALID_VALUE',
                            'message': f'Invalid value for {field}: {str(e)}'
                        }
                    })
                }

        user_id = body.get('user_id', 'anonymous')

        # 예측 수행
        print(f"Predicting car setup for user: {user_id}")
        print(f"User measurements: {user_measure}")

        prediction = predict_car_setup(model, user_measure, feature_cols)

        # 성공 응답
        response_data = {
            'status': 'success',
            'data': prediction,
            'user_id': user_id
        }

        print(f"Prediction complete: {response_data}")

        return {
            'statusCode': 200,
            'headers': {
                'Content-Type': 'application/json',
                'Access-Control-Allow-Origin': '*'
            },
            'body': json.dumps(response_data)
        }

    except ValueError as e:
        print(f"Validation error: {str(e)}")
        return {
            'statusCode': 400,
            'headers': {
                'Content-Type': 'application/json',
                'Access-Control-Allow-Origin': '*'
            },
            'body': json.dumps({
                'status': 'error',
                'error': {
                    'code': 'INVALID_INPUT',
                    'message': str(e)
                }
            })
        }

    except Exception as e:
        print(f"Unexpected error: {str(e)}")
        print(traceback.format_exc())
        return {
            'statusCode': 500,
            'headers': {
                'Content-Type': 'application/json',
                'Access-Control-Allow-Origin': '*'
            },
            'body': json.dumps({
                'status': 'error',
                'error': {
                    'code': 'INTERNAL_ERROR',
                    'message': str(e)
                }
            })
        }
