"""
Lambda Handler for Face Landmark Extraction Service
얼굴 랜드마크 추출 Lambda 함수 (MediaPipe FaceMesh)
"""

import json
import os
import traceback
from typing import List, Tuple, Dict
from urllib.parse import urlparse
import boto3
import cv2
import numpy as np
import mediapipe as mp

# MediaPipe 환경 변수 설정
os.environ['MEDIAPIPE_DISABLE_GPU'] = '1'
os.environ['GLOG_logtostderr'] = '1'
os.environ['TMPDIR'] = '/tmp'

# AWS S3 클라이언트
s3_client = boto3.client('s3')

# MediaPipe FaceMesh 설정
mp_face_mesh = mp.solutions.face_mesh


def parse_indices_arg(indices_str: str) -> List[Tuple[int, int]]:
    """
    인덱스 문자열 파싱
    예: "1230,0561,0072" → [(123, 0), (56, 1), (7, 2)]
    """
    if not indices_str or indices_str.strip() == "":
        return []

    out = []
    for token in indices_str.split(","):
        t = token.strip()
        if len(t) != 4 or not t.isdigit():
            raise ValueError(f"Invalid index token: '{t}' (must be 4 digits like 1230)")

        lm_id = int(t[:3])
        coord_id = int(t[3])

        if not (0 <= lm_id <= 467 and 0 <= coord_id <= 2):
            raise ValueError(f"Out of range: '{t}' (landmark 0-467, coord 0-2)")

        out.append((lm_id, coord_id))

    # 중복 제거
    seen = set()
    uniq = []
    for x in out:
        if x not in seen:
            uniq.append(x)
            seen.add(x)

    return uniq


def all_xyz_indices() -> List[Tuple[int, int]]:
    """모든 468개 랜드마크의 x, y, z 좌표 인덱스 생성"""
    idx = []
    for lm in range(468):
        for c in (0, 1, 2):
            idx.append((lm, c))
    return idx


def index_code(lm_id: int, coord_id: int) -> str:
    """인덱스를 4자리 문자열로 변환 (예: 123, 0 → "1230")"""
    return f"{lm_id:03d}{coord_id:d}"


def download_from_s3(s3_url: str) -> str:
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


def upload_to_s3(local_path: str, bucket: str, key: str) -> str:
    """S3에 파일 업로드"""
    try:
        s3_client.upload_file(
            local_path,
            bucket,
            key,
            ExtraArgs={'ContentType': 'text/plain'}
        )
        region = os.environ.get('AWS_REGION', 'ap-northeast-2')
        return f"https://{bucket}.s3.{region}.amazonaws.com/{key}"
    except Exception as e:
        print(f"S3 upload error: {str(e)}")
        raise RuntimeError(f"Failed to upload {local_path} to {bucket}/{key}: {e}")


def extract_face_landmarks(
    image_path: str,
    indices: List[Tuple[int, int]],
    refine: bool = False
) -> Dict[str, float]:
    """
    이미지에서 얼굴 랜드마크 추출

    Args:
        image_path: 로컬 이미지 경로
        indices: 추출할 랜드마크 인덱스 리스트 (비어있으면 전체)
        refine: FaceMesh refine_landmarks 옵션

    Returns:
        랜드마크 딕셔너리 {"0010": 0.512345, "0011": 0.623456, ...}
    """
    print(f"Processing image: {image_path}")

    # 이미지 로드
    img = cv2.imread(image_path)
    if img is None:
        raise FileNotFoundError(f"Unable to load image: {image_path}")

    h, w = img.shape[:2]
    print(f"Image size: {w}x{h}")

    # 추출할 인덱스 결정
    if not indices:
        indices = all_xyz_indices()

    print(f"Extracting {len(indices)} landmark coordinates")

    # MediaPipe FaceMesh 처리
    with mp_face_mesh.FaceMesh(
        static_image_mode=True,
        refine_landmarks=refine,
        max_num_faces=1,
        min_detection_confidence=0.5
    ) as face_mesh:
        img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        results = face_mesh.process(img_rgb)

    if not results.multi_face_landmarks:
        raise RuntimeError("No face detected. Please use a clear frontal face image.")

    landmarks = results.multi_face_landmarks[0].landmark

    if len(landmarks) < 468:
        raise RuntimeError(f"Insufficient landmarks detected: {len(landmarks)} < 468")

    print(f"Detected {len(landmarks)} face landmarks")

    # 랜드마크 값 추출
    values: Dict[str, float] = {}

    for lm_id, coord_id in indices:
        if lm_id >= len(landmarks):
            continue

        lm = landmarks[lm_id]
        if coord_id == 0:
            val = lm.x
        elif coord_id == 1:
            val = lm.y
        else:  # coord_id == 2
            val = lm.z

        values[index_code(lm_id, coord_id)] = float(val)

    return values


def write_profile_txt(values: Dict[str, float]) -> str:
    """
    랜드마크 값을 텍스트 파일로 저장 (원본 facedata.py 형식)

    Returns:
        로컬 파일 경로
    """
    output_path = "/tmp/facedata_profile.txt"

    with open(output_path, "w", encoding="utf-8") as f:
        for key in sorted(values.keys()):
            f.write(f"{key} {values[key]:.8f}\n")
        f.write("FFFF")  # 원본 형식과 동일하게 종료 마커

    return output_path


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
        if 'image_url' not in body:
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
                        'message': 'Missing required field: image_url'
                    }
                })
            }

        image_url = body['image_url']
        user_id = body.get('user_id', 'anonymous')
        refine = body.get('refine', False)
        indices_str = body.get('indices', '')

        # 인덱스 파싱
        try:
            indices = parse_indices_arg(indices_str)
            print(f"Parsed {len(indices)} indices from input")
        except ValueError as e:
            return {
                'statusCode': 400,
                'headers': {
                    'Content-Type': 'application/json',
                    'Access-Control-Allow-Origin': '*'
                },
                'body': json.dumps({
                    'status': 'error',
                    'error': {
                        'code': 'INVALID_INDICES',
                        'message': str(e)
                    }
                })
            }

        # S3에서 이미지 다운로드
        print(f"Downloading image for user: {user_id}")
        local_image = download_from_s3(image_url)

        # 얼굴 랜드마크 추출
        print(f"Extracting face landmarks (refine={refine})")
        landmarks = extract_face_landmarks(local_image, indices, refine)

        # 텍스트 파일 생성
        profile_txt = write_profile_txt(landmarks)

        # 결과 파일 S3에 업로드
        result_bucket = os.environ.get('RESULT_BUCKET', 'hypermob-results')
        result_key = f"facedata/{user_id}/profile_{context.aws_request_id}.txt"

        print(f"Uploading result to s3://{result_bucket}/{result_key}")
        result_url = upload_to_s3(profile_txt, result_bucket, result_key)

        # 임시 파일 정리
        try:
            os.remove(local_image)
            os.remove(profile_txt)
        except:
            pass

        # 성공 응답
        response_data = {
            'status': 'success',
            'data': {
                'landmarks': landmarks,
                'landmark_count': len(landmarks),
                'output_file_url': result_url,
                'user_id': user_id
            }
        }

        print(f"Face landmark extraction complete: {len(landmarks)} coordinates")

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
                    'code': 'FACE_DETECTION_FAILED',
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
