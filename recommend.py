import json
import numpy as np
import pandas as pd
import joblib
from sklearn.pipeline import Pipeline
from sklearn.preprocessing import StandardScaler
from sklearn.linear_model import RidgeCV, Ridge
from sklearn.model_selection import KFold

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

def load_data(json_path):
    with open(json_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    df = pd.DataFrame(data)

    feature_cols = ["height", "upper_arm", "forearm", "thigh", "calf", "torso"]
    target_cols  = ["seat_position", "seat_angle", "seat_front_height", "seat_rear_height",
                    "mirror_left_yaw", "mirror_left_pitch", "mirror_right_yaw", "mirror_right_pitch",
                    "mirror_room_yaw", "mirror_room_pitch", "wheel_position", "wheel_angle"]

    X = df[feature_cols].values
    y = df[target_cols].values
    return X, y, feature_cols, target_cols

def train_model(X, y):
    alphas = np.logspace(-3, 3, 13)

    n_samples = len(X)
    if n_samples < 3:
        ridge = Ridge(alpha=1.0)
        pipe = Pipeline([
            ("scaler", StandardScaler()),
            ("ridge", ridge)
        ])
        pipe.fit(X, y)
        return pipe
    
    cv_splits = min(5, n_samples)
    kf = KFold(n_splits=cv_splits, shuffle=True, random_state=42)
    ridge = RidgeCV(alphas=alphas, cv=kf, scoring='neg_mean_squared_error')

    pipe = Pipeline([
        ("scaler", StandardScaler()),
        ("ridge", ridge)
    ])

    pipe.fit(X, y)
    return pipe

def predict_car_fit(model, user_measure, feature_cols):
    x_new = np.array([[user_measure[c] for c in feature_cols]])
    raw_pred = model.predict(x_new)[0]

    target_order = [
        "seat_position", "seat_angle", "seat_front_height", "seat_rear_height",
        "mirror_left_yaw", "mirror_left_pitch", "mirror_right_yaw", "mirror_right_pitch",
        "mirror_room_yaw", "mirror_room_pitch", "wheel_position", "wheel_angle"
    ]

    clipped_val = {}

    for i, key in enumerate(target_order):
        low, high = TARGET_RANGES[key]
        clipped_val[key] = float(np.clip(raw_pred[i], low, high))

    return {
        "seat_position": int(clipped_val["seat_position"]),
        "seat_angle": int(clipped_val["seat_angle"]),
        "seat_front_height": int(clipped_val["seat_front_height"]),
        "seat_rear_height": int(clipped_val["seat_rear_height"]),
        "mirror_left_yaw": int(clipped_val["mirror_left_yaw"]),
        "mirror_left_pitch": int(clipped_val["mirror_left_pitch"]),
        "mirror_right_yaw": int(clipped_val["mirror_right_yaw"]),
        "mirror_right_pitch": int(clipped_val["mirror_right_pitch"]),
        "mirror_room_yaw": int(clipped_val["mirror_room_yaw"]),
        "mirror_room_pitch": int(clipped_val["mirror_room_pitch"]),
        "wheel_position": int(clipped_val["wheel_position"]),
        "wheel_angle": int(clipped_val["wheel_angle"])
    }

def save_model(model, path="car_fit_model.joblib"):
    joblib.dump(model, path)

def load_model(path="car_fit_model.joblib"):
    return joblib.load(path)

if __name__ == "__main__":  # Example usage
    X, y, feature_cols, target_cols = load_data("json_template.json")   #json_template.json 자리에 파일 경로 넣기

    model = train_model(X, y)

    new_user = {
        "height": 175.0, "upper_arm": 31.0, "forearm": 26.0,
        "thigh": 51.0, "calf": 36.0, "torso": 61.0
    }

    prediction = predict_car_fit(model, new_user, feature_cols)
    print("Prediction:", prediction)

    save_model(model, "car_fit_model.joblib")