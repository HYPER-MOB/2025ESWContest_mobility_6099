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
DB_NAME = os.environ.get('DB_NAME', 'mydrive3dx')

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


def create_profile(body):
    """프로필 생성"""
    conn = get_db_connection()

    try:
        with conn.cursor() as cursor:
            # 필수 필드 검증
            user_id = body.get('userId')
            if not user_id:
                raise ValueError("userId is required")

            # 프로필 ID 생성
            profile_id = f"profile_{uuid.uuid4().hex[:12]}"

            # 체형 데이터
            sitting_height = body.get('sittingHeight', 0)
            shoulder_width = body.get('shoulderWidth', 0)
            arm_length = body.get('armLength', 0)
            head_height = body.get('headHeight', 0)
            eye_height = body.get('eyeHeight', 0)
            leg_length = body.get('legLength', 0)
            torso_length = body.get('torsoLength', 0)
            weight = body.get('weight', 0)
            height = body.get('height', 0)

            # 차량 ID (옵션)
            vehicle_id = body.get('vehicleId')

            # profiles 테이블에 삽입
            insert_query = """
                INSERT INTO profiles (
                    profile_id, user_id, vehicle_id,
                    sitting_height, shoulder_width, arm_length,
                    head_height, eye_height, leg_length, torso_length,
                    weight, height,
                    created_at, updated_at
                ) VALUES (
                    %s, %s, %s,
                    %s, %s, %s,
                    %s, %s, %s, %s,
                    %s, %s,
                    NOW(), NOW()
                )
            """

            cursor.execute(insert_query, (
                profile_id, user_id, vehicle_id,
                sitting_height, shoulder_width, arm_length,
                head_height, eye_height, leg_length, torso_length,
                weight, height
            ))

            conn.commit()

            print(f"Profile created: {profile_id} for user: {user_id}")

            # 생성된 프로필 조회
            cursor.execute("""
                SELECT profile_id, user_id, vehicle_id,
                       sitting_height, shoulder_width, arm_length,
                       head_height, eye_height, leg_length, torso_length,
                       weight, height,
                       created_at, updated_at
                FROM profiles
                WHERE profile_id = %s
            """, (profile_id,))

            profile = cursor.fetchone()

            return {
                'statusCode': 201,
                'headers': {
                    'Content-Type': 'application/json',
                    'Access-Control-Allow-Origin': '*'
                },
                'body': json.dumps({
                    'status': 'success',
                    'data': {
                        'profile_id': profile['profile_id'],
                        'user_id': profile['user_id'],
                        'vehicle_id': profile['vehicle_id'],
                        'body_measurements': {
                            'sitting_height': float(profile['sitting_height']),
                            'shoulder_width': float(profile['shoulder_width']),
                            'arm_length': float(profile['arm_length']),
                            'head_height': float(profile['head_height']),
                            'eye_height': float(profile['eye_height']),
                            'leg_length': float(profile['leg_length']),
                            'torso_length': float(profile['torso_length']),
                            'weight': float(profile['weight']),
                            'height': float(profile['height'])
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


def handler(event, context):
    """Lambda 핸들러"""
    print(f"Event: {json.dumps(event)}")

    try:
        # HTTP 메서드 확인
        http_method = event.get('httpMethod', event.get('requestContext', {}).get('http', {}).get('method', 'POST'))

        # API Gateway 프록시 통합 처리
        if 'body' in event:
            if isinstance(event['body'], str):
                body = json.loads(event['body'])
            else:
                body = event['body']
        else:
            body = event

        # POST /profiles/body-scan - 프로필 생성
        if http_method == 'POST':
            return create_profile(body)

        # 지원하지 않는 메서드
        else:
            return {
                'statusCode': 405,
                'headers': {
                    'Content-Type': 'application/json',
                    'Access-Control-Allow-Origin': '*'
                },
                'body': json.dumps({
                    'status': 'error',
                    'error': {
                        'code': 'METHOD_NOT_ALLOWED',
                        'message': f'Method {http_method} not allowed'
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
