const mysql = require('mysql2/promise');
const dbConfig = require('./db');

/**
 * HYPERMOB User Profile Lambda
 * 사용자 프로필 조회 및 관리
 */
exports.handler = async (event) => {
  console.log('Received event:', JSON.stringify(event, null, 2));

  let connection;

  try {
    connection = await mysql.createConnection(dbConfig);

    // GET /users/{user_id}/profile - 프로필 조회
    if (event.httpMethod === 'GET' && event.pathParameters?.user_id) {
      return await getUserProfile(connection, event.pathParameters.user_id);
    }

    // PUT /users/{user_id}/profile - 프로필 생성/업데이트
    if (event.httpMethod === 'PUT' && event.pathParameters?.user_id) {
      return await upsertUserProfile(connection, event);
    }

    // DELETE /users/{user_id}/profile - 프로필 삭제
    if (event.httpMethod === 'DELETE' && event.pathParameters?.user_id) {
      return await deleteUserProfile(connection, event.pathParameters.user_id);
    }

    // 지원하지 않는 경로
    return {
      statusCode: 404,
      headers: {
        'Content-Type': 'application/json',
        'Access-Control-Allow-Origin': '*'
      },
      body: JSON.stringify({
        error: 'not_found',
        message: '요청한 경로를 찾을 수 없습니다'
      })
    };

  } catch (error) {
    console.error('Error:', error);
    return {
      statusCode: 500,
      headers: {
        'Content-Type': 'application/json',
        'Access-Control-Allow-Origin': '*'
      },
      body: JSON.stringify({
        error: 'internal_server_error',
        message: '서버 오류가 발생했습니다',
        details: process.env.NODE_ENV === 'development' ? error.message : undefined
      })
    };
  } finally {
    if (connection) {
      await connection.end();
    }
  }
};

/**
 * 사용자 프로필 조회
 */
async function getUserProfile(connection, userId) {
  const [rows] = await connection.execute(
    'SELECT * FROM user_profiles WHERE user_id = ?',
    [userId]
  );

  if (rows.length === 0) {
    return {
      statusCode: 200,
      headers: {
        'Content-Type': 'application/json',
        'Access-Control-Allow-Origin': '*'
      },
      body: JSON.stringify({
        status: 'success',
        data: {
          user_id: userId,
          has_profile: false,
          message: '사용자 프로필이 존재하지 않습니다'
        }
      })
    };
  }

  const profile = rows[0];

  // JSON 필드 파싱
  const responseData = {
    user_id: profile.user_id,
    body_measurements: typeof profile.body_measurements === 'string'
      ? JSON.parse(profile.body_measurements)
      : profile.body_measurements,
    vehicle_settings: typeof profile.vehicle_settings === 'string'
      ? JSON.parse(profile.vehicle_settings)
      : profile.vehicle_settings,
    has_profile: Boolean(profile.has_profile),
    created_at: profile.created_at,
    updated_at: profile.updated_at
  };

  return {
    statusCode: 200,
    headers: {
      'Content-Type': 'application/json',
      'Access-Control-Allow-Origin': '*'
    },
    body: JSON.stringify({
      status: 'success',
      data: responseData
    })
  };
}

/**
 * 사용자 프로필 생성 또는 업데이트
 */
async function upsertUserProfile(connection, event) {
  const userId = event.pathParameters.user_id;
  const body = JSON.parse(event.body);
  const { body_measurements, vehicle_settings } = body;

  // 입력 검증
  if (!body_measurements || !vehicle_settings) {
    return {
      statusCode: 400,
      headers: {
        'Content-Type': 'application/json',
        'Access-Control-Allow-Origin': '*'
      },
      body: JSON.stringify({
        error: 'invalid_request',
        message: 'body_measurements와 vehicle_settings가 필요합니다'
      })
    };
  }

  // body_measurements 필드 검증
  const requiredBodyFields = ['height', 'upper_arm', 'forearm', 'thigh', 'calf', 'torso'];
  for (const field of requiredBodyFields) {
    if (typeof body_measurements[field] !== 'number') {
      return {
        statusCode: 400,
        headers: {
          'Content-Type': 'application/json',
          'Access-Control-Allow-Origin': '*'
        },
        body: JSON.stringify({
          error: 'invalid_body_measurements',
          message: `body_measurements.${field}는 숫자여야 합니다`
        })
      };
    }
  }

  // vehicle_settings 필드 검증
  const requiredVehicleFields = [
    'seat_position', 'seat_angle', 'seat_front_height', 'seat_rear_height',
    'mirror_left_yaw', 'mirror_left_pitch', 'mirror_right_yaw', 'mirror_right_pitch',
    'mirror_room_yaw', 'mirror_room_pitch', 'wheel_position', 'wheel_angle'
  ];
  for (const field of requiredVehicleFields) {
    if (typeof vehicle_settings[field] !== 'number') {
      return {
        statusCode: 400,
        headers: {
          'Content-Type': 'application/json',
          'Access-Control-Allow-Origin': '*'
        },
        body: JSON.stringify({
          error: 'invalid_vehicle_settings',
          message: `vehicle_settings.${field}는 숫자여야 합니다`
        })
      };
    }
  }

  // 프로필 존재 여부 확인
  const [existing] = await connection.execute(
    'SELECT user_id FROM user_profiles WHERE user_id = ?',
    [userId]
  );

  let isNewProfile = false;

  if (existing.length === 0) {
    // 신규 생성
    await connection.execute(
      `INSERT INTO user_profiles
       (user_id, body_measurements, vehicle_settings, has_profile)
       VALUES (?, ?, ?, TRUE)`,
      [userId, JSON.stringify(body_measurements), JSON.stringify(vehicle_settings)]
    );
    isNewProfile = true;
  } else {
    // 업데이트
    await connection.execute(
      `UPDATE user_profiles
       SET body_measurements = ?, vehicle_settings = ?, has_profile = TRUE,
           updated_at = CURRENT_TIMESTAMP
       WHERE user_id = ?`,
      [JSON.stringify(body_measurements), JSON.stringify(vehicle_settings), userId]
    );
  }

  return {
    statusCode: isNewProfile ? 201 : 200,
    headers: {
      'Content-Type': 'application/json',
      'Access-Control-Allow-Origin': '*'
    },
    body: JSON.stringify({
      status: 'success',
      data: {
        user_id: userId,
        body_measurements,
        vehicle_settings,
        has_profile: true,
        action: isNewProfile ? 'created' : 'updated'
      }
    })
  };
}

/**
 * 사용자 프로필 삭제
 */
async function deleteUserProfile(connection, userId) {
  const [result] = await connection.execute(
    'DELETE FROM user_profiles WHERE user_id = ?',
    [userId]
  );

  if (result.affectedRows === 0) {
    return {
      statusCode: 404,
      headers: {
        'Content-Type': 'application/json',
        'Access-Control-Allow-Origin': '*'
      },
      body: JSON.stringify({
        error: 'profile_not_found',
        message: '프로필을 찾을 수 없습니다'
      })
    };
  }

  return {
    statusCode: 200,
    headers: {
      'Content-Type': 'application/json',
      'Access-Control-Allow-Origin': '*'
    },
    body: JSON.stringify({
      status: 'success',
      message: '프로필이 삭제되었습니다'
    })
  };
}
