import time
from math import atan2, degrees

import cv2
import mediapipe as mp
import numpy as np

EYE_AR_THRESH = 0.20
EYE_CLOSED_MIN_SEC = 1.5
ROLL_DEG_THRESH = 35.0
PITCH_DEG_THRESH = 25.0
HEAD_TILT_MIN_SEC = 1.5
YAWN_MAR_THRESH = 0.60
YAWN_MIN_SEC = 1.2
PRINT_INTERVAL_SEC = 0.5
SMOOTHING = 0.6

mp_face_mesh = mp.solutions.face_mesh
face_mesh = mp_face_mesh.FaceMesh(
    max_num_faces=1,
    refine_landmarks=True,
    min_detection_confidence=0.5,
    min_tracking_confidence=0.5,
)

LEFT_EYE = [33, 160, 158, 133, 153, 144]
RIGHT_EYE = [362, 385, 387, 263, 373, 380]
MOUTH_TOP = 13
MOUTH_BOTTOM = 14
MOUTH_LEFT = 78
MOUTH_RIGHT = 308
LEFT_EYE_OUTER = 33
LEFT_EYE_INNER = 133
RIGHT_EYE_OUTER = 362
RIGHT_EYE_INNER = 263
MOUTH_CENTER_REF = [13, 14, 78, 308]

def euclidean(p1, p2):
    return float(np.linalg.norm(np.array(p1) - np.array(p2)))

def eye_aspect_ratio(landmarks, eye_idx):
    p1, p2, p3, p4, p5, p6 = [landmarks[i] for i in eye_idx]
    vertical = euclidean(p2, p6) + euclidean(p3, p5)
    horizontal = 2.0 * euclidean(p1, p4)
    if horizontal == 0.0:
        return 0.0
    return vertical / horizontal

def mouth_aspect_ratio(landmarks):
    top = landmarks[MOUTH_TOP]
    bottom = landmarks[MOUTH_BOTTOM]
    left = landmarks[MOUTH_LEFT]
    right = landmarks[MOUTH_RIGHT]
    den = euclidean(left, right)
    if den == 0.0:
        return 0.0
    return euclidean(top, bottom) / den

def get_point(landmarks, idx):
    return np.array(landmarks[idx], dtype=np.float32)

def center_of(landmarks, idx_list):
    pts = np.array([landmarks[i] for i in idx_list], dtype=np.float32)
    return pts.mean(axis=0)

def estimate_head_angles(landmarks):
    left_eye_center = (get_point(landmarks, LEFT_EYE_OUTER) + get_point(landmarks, LEFT_EYE_INNER)) / 2.0
    right_eye_center = (get_point(landmarks, RIGHT_EYE_OUTER) + get_point(landmarks, RIGHT_EYE_INNER)) / 2.0
    eye_mid = (left_eye_center + right_eye_center) / 2.0
    mouth_center = center_of(landmarks, MOUTH_CENTER_REF)

    dx = float(right_eye_center[0] - left_eye_center[0])
    dy = float(right_eye_center[1] - left_eye_center[1])
    roll_rad = atan2(dy, dx)
    roll_deg = degrees(roll_rad)

    v = mouth_center - eye_mid
    pitch_from_vertical_rad = atan2(abs(float(v[0])), max(1e-6, float(v[1])))
    pitch_deg = degrees(pitch_from_vertical_rad)

    return float(roll_deg), float(pitch_deg)

def exp_smooth(prev, new, alpha=SMOOTHING):
    if prev is None:
        return new
    return alpha * new + (1.0 - alpha) * prev

def main():
    cap = cv2.VideoCapture(0)
    try:
        with open("drowsiness.txt", "w", encoding="utf-8") as f:
            f.write("2\n")
    except Exception:
        pass

    if not cap.isOpened():
        print("Cannot open camera.")
        return

    last_print_t = 0.0
    eye_closed_start = None
    head_tilt_start = None
    yawn_start = None
    smooth_EAR = None
    smooth_MAR = None
    smooth_roll = None
    smooth_pitch = None

    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                print("Cannot read frame.")
                break

            h, w = frame.shape[:2]
            rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            result = face_mesh.process(rgb)
            now = time.time()

            status_flags = {"EYES_CLOSED": False, "HEAD_TILT": False, "YAWNING": False}

            if result.multi_face_landmarks:
                lm = result.multi_face_landmarks[0].landmark
                landmarks = [(lm_i.x * w, lm_i.y * h) for lm_i in lm]

                ear_left = eye_aspect_ratio(landmarks, LEFT_EYE)
                ear_right = eye_aspect_ratio(landmarks, RIGHT_EYE)
                ear = (ear_left + ear_right) / 2.0
                smooth_EAR = exp_smooth(smooth_EAR, ear)

                mar = mouth_aspect_ratio(landmarks)
                smooth_MAR = exp_smooth(smooth_MAR, mar)

                roll_deg, pitch_deg = estimate_head_angles(landmarks)
                smooth_roll = exp_smooth(smooth_roll, abs(roll_deg))
                smooth_pitch = exp_smooth(smooth_pitch, pitch_deg)

                if smooth_EAR is not None and smooth_EAR < EYE_AR_THRESH:
                    if eye_closed_start is None:
                        eye_closed_start = now
                    if now - eye_closed_start >= EYE_CLOSED_MIN_SEC:
                        status_flags["EYES_CLOSED"] = True
                else:
                    eye_closed_start = None

                if (smooth_roll is not None and smooth_roll > ROLL_DEG_THRESH) or (
                    smooth_pitch is not None and smooth_pitch > PITCH_DEG_THRESH
                ):
                    if head_tilt_start is None:
                        head_tilt_start = now
                    if now - head_tilt_start >= HEAD_TILT_MIN_SEC:
                        status_flags["HEAD_TILT"] = True
                else:
                    head_tilt_start = None

                if smooth_MAR is not None and smooth_MAR > YAWN_MAR_THRESH:
                    if yawn_start is None:
                        yawn_start = now
                    if now - yawn_start >= YAWN_MIN_SEC:
                        status_flags["YAWNING"] = True
                else:
                    yawn_start = None

            if now - last_print_t >= PRINT_INTERVAL_SEC:
                last_print_t = now
                reasons = []
                if status_flags["EYES_CLOSED"]:
                    reasons.append("Eyes closed")
                if status_flags["HEAD_TILT"]:
                    reasons.append("Head tilted")
                if status_flags["YAWNING"]:
                    reasons.append("Yawning")
                level = "DROWSY" if any(reasons) else "OK"

                debug_str = ""
                if smooth_EAR is not None:
                    debug_str += f" EAR={smooth_EAR:.3f}"
                if smooth_MAR is not None:
                    debug_str += f" MAR={smooth_MAR:.3f}"
                if smooth_roll is not None:
                    debug_str += f" ROLL≈{smooth_roll:.1f}°"
                if smooth_pitch is not None:
                    debug_str += f" PITCH≈{smooth_pitch:.1f}°"

                is_drowsy = any(reasons)
                try:
                    with open("drowsiness.txt", "w", encoding="utf-8") as f:
                        f.write("1\n" if is_drowsy else "0\n")
                except Exception:
                    pass

                if is_drowsy:
                    print(f"[{time.strftime('%H:%M:%S')}] Status={level} / Reason={', '.join(reasons)} |{debug_str}")
                else:
                    print(f"[{time.strftime('%H:%M:%S')}] Status={level} |{debug_str}")

    except KeyboardInterrupt:
        print("\nTerminated.")
    finally:
        try:
            face_mesh.close()
        except Exception:
            pass
        cap.release()
        cv2.destroyAllWindows()

if __name__ == "__main__":
    main()