"""
Lambda Handler for Body Measurement Service
신체 측정 Lambda 함수
"""

import json
import os
import traceback
from urllib.parse import urlparse
import boto3
import cv2
import numpy as np
import mediapipe as mp

# MediaPipe 환경 변수 설정 (read-only 파일시스템 문제 해결)
os.environ['MEDIAPIPE_DISABLE_GPU'] = '1'
os.environ['GLOG_logtostderr'] = '1'
os.environ['TMPDIR'] = '/tmp'

# AWS S3 클라이언트
s3_client = boto3.client('s3')

# MediaPipe 설정
mp_pose = mp.solutions.pose
mp_drawing = mp.solutions.drawing_utils
mp_styles = mp.solutions.drawing_styles

POSE_LM = mp_pose.PoseLandmark

# 측정 기준점
TOP_CANDIDATES = [
    POSE_LM.NOSE, POSE_LM.LEFT_EYE, POSE_LM.RIGHT_EYE,
    POSE_LM.LEFT_EAR, POSE_LM.RIGHT_EAR
]
BOTTOM_CANDIDATES = [
    POSE_LM.LEFT_HEEL, POSE_LM.RIGHT_HEEL,
    POSE_LM.LEFT_FOOT_INDEX, POSE_LM.RIGHT_FOOT_INDEX
]


def euclid(p1, p2, img_w, img_h):
    """두 랜드마크 간 유클리드 거리 계산"""
    x1, y1 = p1.x * img_w, p1.y * img_h
    x2, y2 = p2.x * img_w, p2.y * img_h
    return float(np.hypot(x2 - x1, y2 - y1))


def safe_get(landmarks, ids):
    """가시성이 높은 랜드마크만 추출"""
    return [landmarks[i] for i in ids if landmarks[i].visibility > 0.5]


def midpoint(p, q):
    """두 랜드마크의 중간점 계산"""
    m = type(p)()
    m.x = (p.x + q.x) / 2.0
    m.y = (p.y + q.y) / 2.0
    m.z = (p.z + q.z) / 2.0
    m.visibility = min(p.visibility, q.visibility)
    return m


def download_from_s3(s3_url):
    """S3 URL에서 이미지 다운로드"""
    try:
        parsed = urlparse(s3_url)

        # s3:// 형식 처리
        if parsed.scheme == 's3':
            bucket = parsed.netloc
            key = parsed.path.lstrip('/')
        # https://bucket.s3.region.amazonaws.com/key 형식 처리
        elif 's3' in parsed.netloc:
            bucket = parsed.netloc.split('.')[0]
            key = parsed.path.lstrip('/')
        else:
            raise ValueError(f"Invalid S3 URL format: {s3_url}")

        local_path = f"/tmp/{os.path.basename(key)}"
        print(f"Downloading from bucket={bucket}, key={key}")
        s3_client.download_file(bucket, key, local_path)
        return local_path
    except Exception as e:
        print(f"S3 download error: {str(e)}")
        raise


def upload_to_s3(local_path, bucket, key):
    """S3에 파일 업로드"""
    try:
        # ACL 없이 업로드 (S3 버킷의 ACL 비활성화 설정 때문)
        s3_client.upload_file(
            local_path,
            bucket,
            key,
            ExtraArgs={'ContentType': 'image/jpeg'}
        )
        region = os.environ.get('AWS_REGION', 'ap-northeast-2')
        return f"https://{bucket}.s3.{region}.amazonaws.com/{key}"
    except Exception as e:
        print(f"S3 upload error: {str(e)}")
        raise RuntimeError(f"Failed to upload {local_path} to {bucket}/{key}: {e}")


def arm_length(lm, side, w, h):
    """팔 길이 측정 (상완, 전완 분리 반환)"""
    SHOULDER = getattr(POSE_LM, f"{side}_SHOULDER")
    ELBOW = getattr(POSE_LM, f"{side}_ELBOW")
    WRIST = getattr(POSE_LM, f"{side}_WRIST")

    if min(lm[SHOULDER].visibility, lm[ELBOW].visibility, lm[WRIST].visibility) < 0.5:
        return None

    upper = euclid(lm[SHOULDER], lm[ELBOW], w, h)
    lower = euclid(lm[ELBOW], lm[WRIST], w, h)
    return upper, lower  # 상완, 전완 분리 반환


def leg_length(lm, side, w, h):
    """다리 길이 측정 (허벅지, 종아리 분리 반환)"""
    HIP = getattr(POSE_LM, f"{side}_HIP")
    KNEE = getattr(POSE_LM, f"{side}_KNEE")
    ANKLE = getattr(POSE_LM, f"{side}_ANKLE")

    if min(lm[HIP].visibility, lm[KNEE].visibility, lm[ANKLE].visibility) < 0.5:
        return None

    upper = euclid(lm[HIP], lm[KNEE], w, h)
    lower = euclid(lm[KNEE], lm[ANKLE], w, h)
    return upper, lower  # 허벅지, 종아리 분리 반환


def measure_body(image_path, height_cm):
    """신체 측정 수행"""
    print(f"Processing image: {image_path}")

    # 이미지 로드
    img = cv2.imread(image_path)
    if img is None:
        raise FileNotFoundError(f"Unable to load image: {image_path}")

    h, w = img.shape[:2]
    print(f"Image size: {w}x{h}")

    # MediaPipe 포즈 감지
    with mp_pose.Pose(
        static_image_mode=True,
        model_complexity=2,
        enable_segmentation=False
    ) as pose:
        img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        res = pose.process(img_rgb)

    if not res.pose_landmarks:
        raise RuntimeError("Unable to detect pose landmarks. Please use a clear full-body image.")

    lm = res.pose_landmarks.landmark

    # 키 기준점 찾기
    top_pts = safe_get(lm, TOP_CANDIDATES)
    bot_pts = safe_get(lm, BOTTOM_CANDIDATES)

    if not top_pts or not bot_pts:
        raise RuntimeError("Unable to find height reference points (head/feet)")

    # 스케일 계산 (픽셀 -> cm)
    top_y_px = min(p.y for p in top_pts) * h
    bot_y_px = max(p.y for p in bot_pts) * h
    pixel_height = max(1.0, bot_y_px - top_y_px)
    scale = height_cm / pixel_height

    print(f"Scale: {scale:.4f} cm/px")

    # 팔 길이 측정 (상완, 전완 분리)
    left_arm = arm_length(lm, 'LEFT', w, h)
    right_arm = arm_length(lm, 'RIGHT', w, h)

    upper_arm_cm = None
    forearm_cm = None

    if left_arm and right_arm:
        # 양쪽 팔 모두 측정 가능한 경우 평균
        left_upper, left_forearm = left_arm
        right_upper, right_forearm = right_arm
        upper_arm_cm = (left_upper + right_upper) / 2.0 * scale
        forearm_cm = (left_forearm + right_forearm) / 2.0 * scale
    elif left_arm:
        # 왼팔만 측정 가능한 경우
        left_upper, left_forearm = left_arm
        upper_arm_cm = left_upper * scale
        forearm_cm = left_forearm * scale
    elif right_arm:
        # 오른팔만 측정 가능한 경우
        right_upper, right_forearm = right_arm
        upper_arm_cm = right_upper * scale
        forearm_cm = right_forearm * scale

    # 다리 길이 측정 (허벅지, 종아리 분리)
    left_leg = leg_length(lm, 'LEFT', w, h)
    right_leg = leg_length(lm, 'RIGHT', w, h)

    thigh_cm = None
    calf_cm = None

    if left_leg and right_leg:
        # 양쪽 다리 모두 측정 가능한 경우 평균
        left_thigh, left_calf = left_leg
        right_thigh, right_calf = right_leg
        thigh_cm = (left_thigh + right_thigh) / 2.0 * scale
        calf_cm = (left_calf + right_calf) / 2.0 * scale
    elif left_leg:
        # 왼쪽 다리만 측정 가능한 경우
        left_thigh, left_calf = left_leg
        thigh_cm = left_thigh * scale
        calf_cm = left_calf * scale
    elif right_leg:
        # 오른쪽 다리만 측정 가능한 경우
        right_thigh, right_calf = right_leg
        thigh_cm = right_thigh * scale
        calf_cm = right_calf * scale

    # 몸통 길이 측정
    torso_cm = None
    if min(
        lm[POSE_LM.LEFT_SHOULDER].visibility,
        lm[POSE_LM.RIGHT_SHOULDER].visibility,
        lm[POSE_LM.LEFT_HIP].visibility,
        lm[POSE_LM.RIGHT_HIP].visibility
    ) >= 0.5:
        mid_sh = midpoint(lm[POSE_LM.LEFT_SHOULDER], lm[POSE_LM.RIGHT_SHOULDER])
        mid_hip = midpoint(lm[POSE_LM.LEFT_HIP], lm[POSE_LM.RIGHT_HIP])
        torso_cm = euclid(mid_sh, mid_hip, w, h) * scale

    # 오버레이 이미지 생성
    overlay = img.copy()

    # 측정 선 그리기
    def draw_line(p, q, color=(0, 255, 0), thickness=3):
        x1, y1 = int(p.x * w), int(p.y * h)
        x2, y2 = int(q.x * w), int(q.y * h)
        cv2.line(overlay, (x1, y1), (x2, y2), color, thickness)

    # 팔 그리기
    for side in ['LEFT', 'RIGHT']:
        shoulder = getattr(POSE_LM, f"{side}_SHOULDER")
        elbow = getattr(POSE_LM, f"{side}_ELBOW")
        wrist = getattr(POSE_LM, f"{side}_WRIST")

        if all(lm[i].visibility >= 0.5 for i in [shoulder, elbow, wrist]):
            draw_line(lm[shoulder], lm[elbow], color=(0, 255, 0))
            draw_line(lm[elbow], lm[wrist], color=(0, 255, 0))

    # 다리 그리기
    for side in ['LEFT', 'RIGHT']:
        hip = getattr(POSE_LM, f"{side}_HIP")
        knee = getattr(POSE_LM, f"{side}_KNEE")
        ankle = getattr(POSE_LM, f"{side}_ANKLE")

        if all(lm[i].visibility >= 0.5 for i in [hip, knee, ankle]):
            draw_line(lm[hip], lm[knee], color=(255, 0, 0))
            draw_line(lm[knee], lm[ankle], color=(255, 0, 0))

    # 몸통 그리기
    if torso_cm is not None:
        mid_sh = midpoint(lm[POSE_LM.LEFT_SHOULDER], lm[POSE_LM.RIGHT_SHOULDER])
        mid_hip = midpoint(lm[POSE_LM.LEFT_HIP], lm[POSE_LM.RIGHT_HIP])
        draw_line(mid_sh, mid_hip, color=(255, 0, 255), thickness=4)

    # MediaPipe 스켈레톤 그리기
    mp_drawing.draw_landmarks(
        overlay,
        res.pose_landmarks,
        mp_pose.POSE_CONNECTIONS,
        landmark_drawing_spec=mp_styles.get_default_pose_landmarks_style()
    )

    # 결과 이미지 저장
    overlay_path = "/tmp/measurement_overlay.jpg"
    cv2.imwrite(overlay_path, overlay)

    return {
        "scale_cm_per_px": round(scale, 4),
        "upper_arm_cm": round(upper_arm_cm, 1) if upper_arm_cm else None,
        "forearm_cm": round(forearm_cm, 1) if forearm_cm else None,
        "thigh_cm": round(thigh_cm, 1) if thigh_cm else None,
        "calf_cm": round(calf_cm, 1) if calf_cm else None,
        "torso_cm": round(torso_cm, 1) if torso_cm else None,
        "output_image": overlay_path
    }


def handler(event, context):
    """Lambda 핸들러"""
    print(f"Event: {json.dumps(event)}")

    try:
        # API Gateway 프록시 통합 처리
        if 'body' in event:
            if isinstance(event['body'], str):
                body = json.loads(event['body'])
            else:
                body = event['body']
        else:
            body = event

        # 입력 검증
        required_fields = ['image_url', 'height_cm']
        for field in required_fields:
            if field not in body:
                return {
                    'statusCode': 400,
                    'headers': {
                        'Content-Type': 'application/json',
                        'Access-Control-Allow-Origin': '*'
                    },
                    'body': json.dumps({
                        'status': 'error',
                        'error': {
                            'code': 'MISSING_FIELD',
                            'message': f'Missing required field: {field}'
                        }
                    })
                }

        image_url = body['image_url']
        height_cm = float(body['height_cm'])
        user_id = body.get('user_id', 'anonymous')

        # 키 유효성 검사
        if not (100 <= height_cm <= 250):
            return {
                'statusCode': 400,
                'headers': {
                    'Content-Type': 'application/json',
                    'Access-Control-Allow-Origin': '*'
                },
                'body': json.dumps({
                    'status': 'error',
                    'error': {
                        'code': 'INVALID_HEIGHT',
                        'message': 'Height must be between 100 and 250 cm'
                    }
                })
            }

        # S3에서 이미지 다운로드
        print(f"Downloading image for user: {user_id}")
        local_image = download_from_s3(image_url)

        # 신체 측정 수행
        print(f"Measuring body with height: {height_cm} cm")
        result = measure_body(local_image, height_cm)

        # 결과 이미지 S3에 업로드
        result_bucket = os.environ.get('RESULT_BUCKET', 'hypermob-results')
        result_key = f"results/{user_id}/measurement_{context.aws_request_id}.jpg"

        print(f"Uploading result to s3://{result_bucket}/{result_key}")
        result_url = upload_to_s3(result['output_image'], result_bucket, result_key)

        # 임시 파일 정리
        try:
            os.remove(local_image)
            os.remove(result['output_image'])
        except:
            pass

        # 성공 응답
        response_data = {
            'status': 'success',
            'data': {
                'scale_cm_per_px': result['scale_cm_per_px'],
                'upper_arm': result['upper_arm_cm'],
                'forearm': result['forearm_cm'],
                'thigh': result['thigh_cm'],
                'calf': result['calf_cm'],
                'torso': result['torso_cm'],
                'height': height_cm,  # 입력받은 키도 포함 (Recommend API 호출용)
                'output_image_url': result_url,
                'user_id': user_id
            }
        }

        print(f"Measurement complete: {response_data}")

        return {
            'statusCode': 200,
            'headers': {
                'Content-Type': 'application/json',
                'Access-Control-Allow-Origin': '*'
            },
            'body': json.dumps(response_data)
        }

    except FileNotFoundError as e:
        print(f"File not found error: {str(e)}")
        return {
            'statusCode': 400,
            'headers': {
                'Content-Type': 'application/json',
                'Access-Control-Allow-Origin': '*'
            },
            'body': json.dumps({
                'status': 'error',
                'error': {
                    'code': 'IMAGE_NOT_FOUND',
                    'message': str(e)
                }
            })
        }

    except RuntimeError as e:
        print(f"Runtime error: {str(e)}")
        return {
            'statusCode': 400,
            'headers': {
                'Content-Type': 'application/json',
                'Access-Control-Allow-Origin': '*'
            },
            'body': json.dumps({
                'status': 'error',
                'error': {
                    'code': 'POSE_DETECTION_FAILED',
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
