"""
Lambda Handler for Body Scan Profile Management
바디스캔 프로필 관리 Lambda 함수
"""

import json
import os
import traceback
import pymysql
from datetime import datetime
import uuid

# DB 연결 정보 (환경 변수)
DB_HOST = os.environ.get('DB_HOST', 'mydrive3dx.climsg8a689n.ap-northeast-2.rds.amazonaws.com')
DB_USER = os.environ.get('DB_USER', 'admin')
DB_PASSWORD = os.environ.get('DB_PASSWORD', '')
DB_NAME = os.environ.get('DB_NAME', 'hypermob')

# 전역 DB 연결 (Lambda 컨테이너 재사용 최적화)
connection = None


def get_db_connection():
    """DB 연결 가져오기 (재사용)"""
    global connection

    try:
        if connection is None or not connection.open:
            print("Creating new DB connection")
            connection = pymysql.connect(
                host=DB_HOST,
                user=DB_USER,
                password=DB_PASSWORD,
                database=DB_NAME,
                charset='utf8mb4',
                cursorclass=pymysql.cursors.DictCursor,
                connect_timeout=5
            )
            print("DB connection established")
        else:
            # 연결 테스트
            connection.ping(reconnect=True)
            print("Reusing existing DB connection")

        return connection
    except Exception as e:
        print(f"DB connection error: {str(e)}")
        raise


def upsert_profile(user_id, body):
    """프로필 생성 또는 업데이트 (Upsert)"""
    conn = get_db_connection()

    try:
        with conn.cursor() as cursor:
            # user_id로 기존 프로필 조회
            cursor.execute("SELECT profile_id FROM profiles WHERE user_id = %s", (user_id,))
            existing = cursor.fetchone()

            # Body measurements (from body scan)
            body_measurements = body.get('bodyMeasurements', {})
            sitting_height = body_measurements.get('sittingHeight') or body.get('sittingHeight')
            shoulder_width = body_measurements.get('shoulderWidth') or body.get('shoulderWidth')
            arm_length = body_measurements.get('armLength') or body.get('armLength')
            head_height = body_measurements.get('headHeight') or body.get('headHeight')
            eye_height = body_measurements.get('eyeHeight') or body.get('eyeHeight')
            leg_length = body_measurements.get('legLength') or body.get('legLength')
            torso_length = body_measurements.get('torsoLength') or body.get('torsoLength')
            weight = body_measurements.get('weight') or body.get('weight')
            height = body_measurements.get('height') or body.get('height')

            # Vehicle settings (from /recommend API response)
            vehicle_settings = body.get('vehicleSettings', {})
            seat_position = vehicle_settings.get('seat_position')
            seat_angle = vehicle_settings.get('seat_angle')
            seat_front_height = vehicle_settings.get('seat_front_height')
            seat_rear_height = vehicle_settings.get('seat_rear_height')
            mirror_left_yaw = vehicle_settings.get('mirror_left_yaw')
            mirror_left_pitch = vehicle_settings.get('mirror_left_pitch')
            mirror_right_yaw = vehicle_settings.get('mirror_right_yaw')
            mirror_right_pitch = vehicle_settings.get('mirror_right_pitch')
            mirror_room_yaw = vehicle_settings.get('mirror_room_yaw')
            mirror_room_pitch = vehicle_settings.get('mirror_room_pitch')
            wheel_position = vehicle_settings.get('wheel_position')
            wheel_angle = vehicle_settings.get('wheel_angle')

            if existing:
                # UPDATE existing profile
                profile_id = existing['profile_id']
                update_query = """
                    UPDATE profiles SET
                        sitting_height = COALESCE(%s, sitting_height),
                        shoulder_width = COALESCE(%s, shoulder_width),
                        arm_length = COALESCE(%s, arm_length),
                        head_height = COALESCE(%s, head_height),
                        eye_height = COALESCE(%s, eye_height),
                        leg_length = COALESCE(%s, leg_length),
                        torso_length = COALESCE(%s, torso_length),
                        weight = COALESCE(%s, weight),
                        height = COALESCE(%s, height),
                        seat_position = COALESCE(%s, seat_position),
                        seat_angle = COALESCE(%s, seat_angle),
                        seat_front_height = COALESCE(%s, seat_front_height),
                        seat_rear_height = COALESCE(%s, seat_rear_height),
                        mirror_left_yaw = COALESCE(%s, mirror_left_yaw),
                        mirror_left_pitch = COALESCE(%s, mirror_left_pitch),
                        mirror_right_yaw = COALESCE(%s, mirror_right_yaw),
                        mirror_right_pitch = COALESCE(%s, mirror_right_pitch),
                        mirror_room_yaw = COALESCE(%s, mirror_room_yaw),
                        mirror_room_pitch = COALESCE(%s, mirror_room_pitch),
                        wheel_position = COALESCE(%s, wheel_position),
                        wheel_angle = COALESCE(%s, wheel_angle),
                        updated_at = NOW()
                    WHERE user_id = %s
                """
                cursor.execute(update_query, (
                    sitting_height, shoulder_width, arm_length,
                    head_height, eye_height, leg_length, torso_length,
                    weight, height,
                    seat_position, seat_angle, seat_front_height, seat_rear_height,
                    mirror_left_yaw, mirror_left_pitch, mirror_right_yaw, mirror_right_pitch,
                    mirror_room_yaw, mirror_room_pitch, wheel_position, wheel_angle,
                    user_id
                ))
                print(f"Profile updated: {profile_id} for user: {user_id}")
                status_code = 200
            else:
                # INSERT new profile
                profile_id = f"profile_{uuid.uuid4().hex[:12]}"
                insert_query = """
                    INSERT INTO profiles (
                        profile_id, user_id,
                        sitting_height, shoulder_width, arm_length,
                        head_height, eye_height, leg_length, torso_length,
                        weight, height,
                        seat_position, seat_angle, seat_front_height, seat_rear_height,
                        mirror_left_yaw, mirror_left_pitch, mirror_right_yaw, mirror_right_pitch,
                        mirror_room_yaw, mirror_room_pitch, wheel_position, wheel_angle,
                        created_at, updated_at
                    ) VALUES (
                        %s, %s,
                        %s, %s, %s,
                        %s, %s, %s, %s,
                        %s, %s,
                        %s, %s, %s, %s,
                        %s, %s, %s, %s,
                        %s, %s, %s, %s,
                        NOW(), NOW()
                    )
                """
                cursor.execute(insert_query, (
                    profile_id, user_id,
                    sitting_height, shoulder_width, arm_length,
                    head_height, eye_height, leg_length, torso_length,
                    weight, height,
                    seat_position, seat_angle, seat_front_height, seat_rear_height,
                    mirror_left_yaw, mirror_left_pitch, mirror_right_yaw, mirror_right_pitch,
                    mirror_room_yaw, mirror_room_pitch, wheel_position, wheel_angle
                ))
                print(f"Profile created: {profile_id} for user: {user_id}")
                status_code = 201

            conn.commit()

            # 저장된 프로필 조회
            cursor.execute("""
                SELECT profile_id, user_id,
                       sitting_height, shoulder_width, arm_length,
                       head_height, eye_height, leg_length, torso_length,
                       weight, height,
                       seat_position, seat_angle, seat_front_height, seat_rear_height,
                       mirror_left_yaw, mirror_left_pitch, mirror_right_yaw, mirror_right_pitch,
                       mirror_room_yaw, mirror_room_pitch, wheel_position, wheel_angle,
                       created_at, updated_at
                FROM profiles
                WHERE profile_id = %s
            """, (profile_id,))

            profile = cursor.fetchone()

            return {
                'statusCode': status_code,
                'headers': {
                    'Content-Type': 'application/json',
                    'Access-Control-Allow-Origin': '*'
                },
                'body': json.dumps({
                    'status': 'success',
                    'data': {
                        'profile_id': profile['profile_id'],
                        'user_id': profile['user_id'],
                        'body_measurements': {
                            'sitting_height': float(profile['sitting_height']) if profile['sitting_height'] else None,
                            'shoulder_width': float(profile['shoulder_width']) if profile['shoulder_width'] else None,
                            'arm_length': float(profile['arm_length']) if profile['arm_length'] else None,
                            'head_height': float(profile['head_height']) if profile['head_height'] else None,
                            'eye_height': float(profile['eye_height']) if profile['eye_height'] else None,
                            'leg_length': float(profile['leg_length']) if profile['leg_length'] else None,
                            'torso_length': float(profile['torso_length']) if profile['torso_length'] else None,
                            'weight': float(profile['weight']) if profile['weight'] else None,
                            'height': float(profile['height']) if profile['height'] else None
                        },
                        'vehicle_settings': {
                            'seat_position': profile['seat_position'],
                            'seat_angle': profile['seat_angle'],
                            'seat_front_height': profile['seat_front_height'],
                            'seat_rear_height': profile['seat_rear_height'],
                            'mirror_left_yaw': profile['mirror_left_yaw'],
                            'mirror_left_pitch': profile['mirror_left_pitch'],
                            'mirror_right_yaw': profile['mirror_right_yaw'],
                            'mirror_right_pitch': profile['mirror_right_pitch'],
                            'mirror_room_yaw': profile['mirror_room_yaw'],
                            'mirror_room_pitch': profile['mirror_room_pitch'],
                            'wheel_position': profile['wheel_position'],
                            'wheel_angle': profile['wheel_angle']
                        },
                        'created_at': profile['created_at'].isoformat() if profile['created_at'] else None,
                        'updated_at': profile['updated_at'].isoformat() if profile['updated_at'] else None
                    }
                })
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
                    'code': 'VALIDATION_ERROR',
                    'message': str(e)
                }
            })
        }
    except Exception as e:
        conn.rollback()
        print(f"Database error: {str(e)}")
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
                    'code': 'DATABASE_ERROR',
                    'message': str(e)
                }
            })
        }


def get_profile(user_id):
    """프로필 조회"""
    conn = get_db_connection()

    try:
        with conn.cursor() as cursor:
            # user_id로 프로필 조회
            cursor.execute("""
                SELECT profile_id, user_id,
                       sitting_height, shoulder_width, arm_length,
                       head_height, eye_height, leg_length, torso_length,
                       weight, height,
                       seat_position, seat_angle, seat_front_height, seat_rear_height,
                       mirror_left_yaw, mirror_left_pitch, mirror_right_yaw, mirror_right_pitch,
                       mirror_room_yaw, mirror_room_pitch, wheel_position, wheel_angle,
                       created_at, updated_at
                FROM profiles
                WHERE user_id = %s
            """, (user_id,))

            profile = cursor.fetchone()

            if not profile:
                return {
                    'statusCode': 404,
                    'headers': {
                        'Content-Type': 'application/json',
                        'Access-Control-Allow-Origin': '*'
                    },
                    'body': json.dumps({
                        'status': 'error',
                        'error': {
                            'code': 'NOT_FOUND',
                            'message': f'Profile not found for user_id: {user_id}'
                        }
                    })
                }

            return {
                'statusCode': 200,
                'headers': {
                    'Content-Type': 'application/json',
                    'Access-Control-Allow-Origin': '*'
                },
                'body': json.dumps({
                    'status': 'success',
                    'data': {
                        'profile_id': profile['profile_id'],
                        'user_id': profile['user_id'],
                        'body_measurements': {
                            'sitting_height': float(profile['sitting_height']) if profile['sitting_height'] else None,
                            'shoulder_width': float(profile['shoulder_width']) if profile['shoulder_width'] else None,
                            'arm_length': float(profile['arm_length']) if profile['arm_length'] else None,
                            'head_height': float(profile['head_height']) if profile['head_height'] else None,
                            'eye_height': float(profile['eye_height']) if profile['eye_height'] else None,
                            'leg_length': float(profile['leg_length']) if profile['leg_length'] else None,
                            'torso_length': float(profile['torso_length']) if profile['torso_length'] else None,
                            'weight': float(profile['weight']) if profile['weight'] else None,
                            'height': float(profile['height']) if profile['height'] else None
                        },
                        'vehicle_settings': {
                            'seat_position': profile['seat_position'],
                            'seat_angle': profile['seat_angle'],
                            'seat_front_height': profile['seat_front_height'],
                            'seat_rear_height': profile['seat_rear_height'],
                            'mirror_left_yaw': profile['mirror_left_yaw'],
                            'mirror_left_pitch': profile['mirror_left_pitch'],
                            'mirror_right_yaw': profile['mirror_right_yaw'],
                            'mirror_right_pitch': profile['mirror_right_pitch'],
                            'mirror_room_yaw': profile['mirror_room_yaw'],
                            'mirror_room_pitch': profile['mirror_room_pitch'],
                            'wheel_position': profile['wheel_position'],
                            'wheel_angle': profile['wheel_angle']
                        },
                        'created_at': profile['created_at'].isoformat() if profile['created_at'] else None,
                        'updated_at': profile['updated_at'].isoformat() if profile['updated_at'] else None
                    }
                })
            }

    except Exception as e:
        print(f"Database error: {str(e)}")
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
                    'code': 'DATABASE_ERROR',
                    'message': str(e)
                }
            })
        }


def lambda_handler(event, context):
    """Lambda 핸들러"""
    print(f"Event: {json.dumps(event)}")

    try:
        # HTTP 메서드 확인
        http_method = event.get('httpMethod', event.get('requestContext', {}).get('http', {}).get('method', 'POST'))

        # Path parameter에서 user_id 추출
        path_parameters = event.get('pathParameters', {})
        user_id = path_parameters.get('user_id') if path_parameters else None

        # API Gateway 프록시 통합 처리
        if 'body' in event:
            if isinstance(event['body'], str):
                body = json.loads(event['body'])
            else:
                body = event['body']
        else:
            body = {}

        # GET /profiles/{user_id} - 프로필 조회
        if http_method == 'GET' and user_id:
            return get_profile(user_id)

        # POST /profiles/{user_id} - 프로필 생성/업데이트
        if http_method == 'POST' and user_id:
            return upsert_profile(user_id, body)

        # 지원하지 않는 메서드 또는 경로
        else:
            return {
                'statusCode': 400,
                'headers': {
                    'Content-Type': 'application/json',
                    'Access-Control-Allow-Origin': '*'
                },
                'body': json.dumps({
                    'status': 'error',
                    'error': {
                        'code': 'BAD_REQUEST',
                        'message': 'Invalid request. Use GET or POST /profiles/{user_id}'
                    }
                })
            }

    except Exception as e:
        print(f"Handler error: {str(e)}")
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


# Alias for backward compatibility
handler = lambda_handler
