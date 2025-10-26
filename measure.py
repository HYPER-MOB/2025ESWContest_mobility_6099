import cv2
import mediapipe as mp
import numpy as np

image_path = "full_body.jpg"    # 분석할 전신 이미지 경로
height_cm = 168.0   # 사용자의 실제 신장 입력 (cm)

mp_pose = mp.solutions.pose
mp_drawing = mp.solutions.drawing_utils
mp_styles = mp.solutions.drawing_styles

POSE_LM = mp_pose.PoseLandmark

def euclid(p1, p2, img_w, img_h):
    x1, y1 = p1.x * img_w, p1.y * img_h
    x2, y2 = p2.x * img_w, p2.y * img_h
    return float(np.hypot(x2 - x1, y2 - y1))

TOP_CANDIDATES = [
    POSE_LM.NOSE, POSE_LM.LEFT_EYE, POSE_LM.RIGHT_EYE,
    POSE_LM.LEFT_EAR, POSE_LM.RIGHT_EAR
]
BOTTOM_CANDIDATES = [
    POSE_LM.LEFT_HEEL, POSE_LM.RIGHT_HEEL,
    POSE_LM.LEFT_FOOT_INDEX, POSE_LM.RIGHT_FOOT_INDEX
]

def safe_get(landmarks, ids):
    return [landmarks[i] for i in ids if landmarks[i].visibility > 0.5]

def midpoint(p, q):
    m = type(p)()
    m.x = (p.x + q.x) / 2.0
    m.y = (p.y + q.y) / 2.0
    m.z = (p.z + q.z) / 2.0
    m.visibility = min(p.visibility, q.visibility)
    return m

img = cv2.imread(image_path)
if img is None:
    raise FileNotFoundError(f"Unable to find image: {image_path}")
h, w = img.shape[:2]

with mp_pose.Pose(static_image_mode=True, model_complexity=2, enable_segmentation=False) as pose:
    img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    res = pose.process(img_rgb)

if not res.pose_landmarks:
    raise RuntimeError("Unable to find pose landmarks. Please try a clearer full-body image.")

lm = res.pose_landmarks.landmark

top_pts = safe_get(lm, TOP_CANDIDATES)
bot_pts = safe_get(lm, BOTTOM_CANDIDATES)
if not top_pts or not bot_pts:
    raise RuntimeError("Unable to find height reference points (head/feet). Please try a different image.")

top_y_px = min(p.y for p in top_pts) * h
bot_y_px = max(p.y for p in bot_pts) * h
pixel_height = max(1.0, bot_y_px - top_y_px)
scale = height_cm / pixel_height

def arm_length(side='LEFT'):
    SHOULDER = getattr(POSE_LM, f"{side}_SHOULDER")
    ELBOW    = getattr(POSE_LM, f"{side}_ELBOW")
    WRIST    = getattr(POSE_LM, f"{side}_WRIST")
    if min(lm[SHOULDER].visibility, lm[ELBOW].visibility, lm[WRIST].visibility) < 0.5:
        return None
    upper = euclid(lm[SHOULDER], lm[ELBOW], w, h)
    lower = euclid(lm[ELBOW], lm[WRIST], w, h)
    return upper * scale, lower * scale

left_upper_arm, left_forearm = arm_length('LEFT')
right_upper_arm, right_forarm = arm_length('RIGHT')
upper_arm_len_cm = None
forearm_len_cm = None
if left_upper_arm and right_upper_arm:
    upper_arm_len_cm = (left_upper_arm + right_upper_arm) / 2.0
    forearm_len_cm = (left_forearm + right_forarm) / 2.0
else:
    upper_arm_len_cm = left_upper_arm or right_upper_arm
    forearm_len_cm = left_forearm or right_forarm

def leg_length(side='LEFT'):
    HIP   = getattr(POSE_LM, f"{side}_HIP")
    KNEE  = getattr(POSE_LM, f"{side}_KNEE")
    ANKLE = getattr(POSE_LM, f"{side}_ANKLE")
    if min(lm[HIP].visibility, lm[KNEE].visibility, lm[ANKLE].visibility) < 0.5:
        return None
    upper = euclid(lm[HIP], lm[KNEE], w, h)
    lower = euclid(lm[KNEE], lm[ANKLE], w, h)
    return upper * scale, lower * scale

left_thigh, left_calf = leg_length('LEFT')
right_thigh, right_calf = leg_length('RIGHT')
thigh_len_cm = None
calf_len_cm = None
if left_thigh and right_thigh:
    thigh_len_cm = (left_thigh + right_thigh) / 2.0
    calf_len_cm = (left_calf + right_calf) / 2.0
else:
    thigh_len_cm = left_thigh or right_thigh
    calf_len_cm = left_calf or right_calf

if min(lm[POSE_LM.LEFT_SHOULDER].visibility, lm[POSE_LM.RIGHT_SHOULDER].visibility,
       lm[POSE_LM.LEFT_HIP].visibility, lm[POSE_LM.RIGHT_HIP].visibility) >= 0.5:
    mid_sh = midpoint(lm[POSE_LM.LEFT_SHOULDER], lm[POSE_LM.RIGHT_SHOULDER])
    mid_hip = midpoint(lm[POSE_LM.LEFT_HIP], lm[POSE_LM.RIGHT_HIP])
    torso_cm = euclid(mid_sh, mid_hip, w, h) * scale
else:
    torso_cm = None

overlay = img.copy()

def draw_line(p, q, color=(0,255,0), thickness=3):
    x1, y1 = int(p.x * w), int(p.y * h)
    x2, y2 = int(q.x * w), int(q.y * h)
    cv2.line(overlay, (x1, y1), (x2, y2), color, thickness)

if all(lm[i].visibility >= 0.5 for i in [POSE_LM.LEFT_SHOULDER, POSE_LM.LEFT_ELBOW, POSE_LM.LEFT_WRIST]):
    draw_line(lm[POSE_LM.LEFT_SHOULDER], lm[POSE_LM.LEFT_ELBOW])
    draw_line(lm[POSE_LM.LEFT_ELBOW], lm[POSE_LM.LEFT_WRIST])
if all(lm[i].visibility >= 0.5 for i in [POSE_LM.RIGHT_SHOULDER, POSE_LM.RIGHT_ELBOW, POSE_LM.RIGHT_WRIST]):
    draw_line(lm[POSE_LM.RIGHT_SHOULDER], lm[POSE_LM.RIGHT_ELBOW])
    draw_line(lm[POSE_LM.RIGHT_ELBOW], lm[POSE_LM.RIGHT_WRIST])

if all(lm[i].visibility >= 0.5 for i in [POSE_LM.LEFT_HIP, POSE_LM.LEFT_KNEE, POSE_LM.LEFT_ANKLE]):
    draw_line(lm[POSE_LM.LEFT_HIP], lm[POSE_LM.LEFT_KNEE])
    draw_line(lm[POSE_LM.LEFT_KNEE], lm[POSE_LM.LEFT_ANKLE])
if all(lm[i].visibility >= 0.5 for i in [POSE_LM.RIGHT_HIP, POSE_LM.RIGHT_KNEE, POSE_LM.RIGHT_ANKLE]):
    draw_line(lm[POSE_LM.RIGHT_HIP], lm[POSE_LM.RIGHT_KNEE])
    draw_line(lm[POSE_LM.RIGHT_KNEE], lm[POSE_LM.RIGHT_ANKLE])

if torso_cm is not None:
    mid_sh = midpoint(lm[POSE_LM.LEFT_SHOULDER], lm[POSE_LM.RIGHT_SHOULDER])
    mid_hip = midpoint(lm[POSE_LM.LEFT_HIP], lm[POSE_LM.RIGHT_HIP])
    draw_line(mid_sh, mid_hip, color=(255,0,0), thickness=4)

mp_drawing.draw_landmarks(
    overlay, res.pose_landmarks, mp_pose.POSE_CONNECTIONS,
    landmark_drawing_spec=mp_styles.get_default_pose_landmarks_style()
)

overlay_path = "measurement_overlay.jpg"     # 결과 이미지 저장 경로
cv2.imwrite(overlay_path, overlay)
print({
    "scale_cm_per_px": round(scale, 4),
    "upper_arm_cm": round(upper_arm_len_cm, 1) if upper_arm_len_cm else None,
    "forearm_cm": round(forearm_len_cm, 1) if forearm_len_cm else None,
    "thigh_cm": round(thigh_len_cm, 1) if thigh_len_cm else None,
    "calf_cm": round(calf_len_cm, 1) if calf_len_cm else None,
    "torso_cm": round(torso_cm, 1) if torso_cm else None,
    "output_image": overlay_path
})