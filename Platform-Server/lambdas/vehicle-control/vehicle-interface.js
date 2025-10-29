/**
 * 차량 인터페이스 모듈
 * 실제 차량 제어 시스템과의 통신을 담당
 *
 * 현재는 모의 구현(Mock)이며, 실제 배포 시 CAN bus 또는
 * 차량 제어 API와 연동하도록 수정 필요
 */

/**
 * 차량 설정 적용 함수
 *
 * @param {string} carId - 차량 ID
 * @param {Object} settings - 적용할 설정값
 * @returns {Promise<Object>} 적용 결과
 */
async function applyVehicleSettings(carId, settings) {
  console.log(`[Vehicle Interface] Applying settings to vehicle ${carId}`);
  console.log('[Vehicle Interface] Settings:', JSON.stringify(settings, null, 2));

  // 모의 지연 (실제 차량 제어 시뮬레이션)
  // 실제 구현에서는 CAN bus 통신 또는 차량 제어 API 호출
  await new Promise(resolve => setTimeout(resolve, 2000));

  // 설정 적용 결과 생성
  const result = {};

  for (const [key, value] of Object.entries(settings)) {
    // 각 설정에 대해 검증 및 적용
    const applyResult = await applySingleSetting(carId, key, value);
    result[key] = applyResult;
  }

  console.log('[Vehicle Interface] Apply completed:', JSON.stringify(result, null, 2));

  return result;
}

/**
 * 개별 설정 적용
 *
 * @param {string} carId - 차량 ID
 * @param {string} settingKey - 설정 키
 * @param {number} value - 설정값
 * @returns {Promise<Object>} 적용 결과
 */
async function applySingleSetting(carId, settingKey, value) {
  // 설정값 범위 검증
  const validationResult = validateSettingValue(settingKey, value);

  if (!validationResult.valid) {
    console.warn(`[Vehicle Interface] Invalid value for ${settingKey}: ${value}`);
    return {
      status: 'failed',
      value: value,
      reason: validationResult.reason
    };
  }

  // 실제 구현에서는 여기서 CAN bus 명령 전송
  // 예: await sendCANCommand(carId, settingKey, value);

  // 모의 성공 응답
  return {
    status: 'applied',
    value: value,
    timestamp: new Date().toISOString()
  };
}

/**
 * 설정값 범위 검증
 *
 * @param {string} settingKey - 설정 키
 * @param {number} value - 설정값
 * @returns {Object} 검증 결과
 */
function validateSettingValue(settingKey, value) {
  // 설정별 유효 범위 정의
  const validRanges = {
    seat_position: { min: 0, max: 100 },
    seat_angle: { min: 0, max: 45 },
    seat_front_height: { min: 0, max: 100 },
    seat_rear_height: { min: 0, max: 100 },
    mirror_left_yaw: { min: 0, max: 360 },
    mirror_left_pitch: { min: -30, max: 30 },
    mirror_right_yaw: { min: 0, max: 360 },
    mirror_right_pitch: { min: -30, max: 30 },
    mirror_room_yaw: { min: 0, max: 360 },
    mirror_room_pitch: { min: -30, max: 30 },
    wheel_position: { min: 0, max: 100 },
    wheel_angle: { min: 0, max: 45 }
  };

  const range = validRanges[settingKey];

  if (!range) {
    return {
      valid: false,
      reason: `Unknown setting key: ${settingKey}`
    };
  }

  if (typeof value !== 'number') {
    return {
      valid: false,
      reason: 'Value must be a number'
    };
  }

  if (value < range.min || value > range.max) {
    return {
      valid: false,
      reason: `Value ${value} out of range [${range.min}, ${range.max}]`
    };
  }

  return { valid: true };
}

/**
 * 차량 상태 조회
 *
 * @param {string} carId - 차량 ID
 * @returns {Promise<Object>} 차량 상태
 */
async function getVehicleStatus(carId) {
  console.log(`[Vehicle Interface] Getting status for vehicle ${carId}`);

  // 모의 차량 상태 응답
  return {
    car_id: carId,
    online: true,
    battery_level: 85,
    location: { lat: 37.5665, lon: 126.9780 },
    timestamp: new Date().toISOString()
  };
}

/**
 * 차량 연결 테스트
 *
 * @param {string} carId - 차량 ID
 * @returns {Promise<boolean>} 연결 여부
 */
async function testVehicleConnection(carId) {
  console.log(`[Vehicle Interface] Testing connection to vehicle ${carId}`);

  // 모의 연결 테스트 (항상 성공)
  await new Promise(resolve => setTimeout(resolve, 500));

  return true;
}

module.exports = {
  applyVehicleSettings,
  applySingleSetting,
  validateSettingValue,
  getVehicleStatus,
  testVehicleConnection
};
