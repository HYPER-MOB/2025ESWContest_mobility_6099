/**
 * 공통 유틸리티 함수
 */

const crypto = require('crypto');

/**
 * UUID 기반 사용자 ID 생성 (16자)
 * @returns {string} 16자 사용자 ID
 */
function generateUserId() {
  return 'U' + crypto.randomBytes(7).toString('hex').toUpperCase();
}

/**
 * SHA256 해시 기반 Face ID 생성 (16자 hex)
 * @param {Buffer} imageData - 이미지 바이너리 데이터
 * @param {string} timestamp - 타임스탬프
 * @returns {string} 16자 Face ID
 */
function generateFaceId(imageData, timestamp = Date.now().toString()) {
  const hash = crypto.createHash('sha256');
  hash.update(imageData);
  hash.update(timestamp);
  return 'F' + hash.digest('hex').substring(0, 15).toUpperCase();
}

/**
 * BLE 해시키 생성 (16자 hex, 8바이트)
 * @param {string} userId - 사용자 ID
 * @param {string} carId - 차량 ID
 * @param {string} nonce - Nonce (32자 hex)
 * @returns {string} 16자 BLE 해시키
 */
function generateHashKey(userId, carId, nonce) {
  const hash = crypto.createHash('sha256');
  hash.update(userId + carId + nonce);
  return hash.digest('hex').substring(0, 16).toUpperCase();
}

/**
 * Nonce 생성 (32자 hex, 16바이트)
 * @returns {string} 32자 hex nonce
 */
function generateNonce() {
  return crypto.randomBytes(16).toString('hex').toUpperCase();
}

/**
 * NFC UID 생성 (32자 hex, user_id 기반 deterministic)
 * @param {string} userId - 사용자 ID
 * @returns {string} 32자 NFC UID
 */
function generateNfcUid(userId) {
  const hash = crypto.createHash('sha256');
  hash.update(userId + 'NFC_SALT_2025');
  return hash.digest('hex').substring(0, 32).toUpperCase();
}

/**
 * 세션 ID 생성 (32자)
 * @param {string} prefix - 접두사 (예: 'SESSION', 'BLE')
 * @returns {string} 세션 ID
 */
function generateSessionId(prefix = 'SESSION') {
  return prefix + '_' + crypto.randomBytes(12).toString('hex').toUpperCase();
}

/**
 * Lambda 응답 생성 헬퍼
 * @param {number} statusCode - HTTP 상태 코드
 * @param {Object} body - 응답 본문
 * @param {Object} headers - 추가 헤더 (선택)
 * @returns {Object} Lambda 응답 객체
 */
function createResponse(statusCode, body, headers = {}) {
  return {
    statusCode,
    headers: {
      'Content-Type': 'application/json',
      'Access-Control-Allow-Origin': '*', // CORS
      'Access-Control-Allow-Headers': 'Content-Type,X-Amz-Date,Authorization,X-Api-Key,X-Amz-Security-Token',
      'Access-Control-Allow-Methods': 'GET,POST,PUT,DELETE,OPTIONS',
      ...headers,
    },
    body: JSON.stringify(body),
  };
}

/**
 * 에러 응답 생성
 * @param {number} statusCode - HTTP 상태 코드
 * @param {string} error - 에러 코드
 * @param {string} message - 에러 메시지
 * @param {Object} details - 추가 상세 정보 (선택)
 * @returns {Object} Lambda 응답 객체
 */
function createErrorResponse(statusCode, error, message, details = null) {
  const body = { error, message };
  if (details) {
    body.details = details;
  }
  return createResponse(statusCode, body);
}

/**
 * multipart/form-data 파싱 (얼굴 이미지 업로드용)
 * @param {string} body - Base64 인코딩된 요청 본문
 * @param {string} contentType - Content-Type 헤더
 * @returns {Object} { fields: {}, files: {} }
 */
function parseMultipartFormData(body, contentType) {
  // Improved multipart/form-data parser for binary data
  const boundary = contentType.split('boundary=')[1];
  if (!boundary) {
    throw new Error('Invalid multipart/form-data: no boundary');
  }

  const boundaryBuffer = Buffer.from(`--${boundary}`, 'binary');
  const bodyBuffer = Buffer.from(body, 'binary');

  const result = { fields: {}, files: {} };
  const parts = [];

  // Split by boundary
  let start = 0;
  let end = bodyBuffer.indexOf(boundaryBuffer, start);

  while (end !== -1) {
    if (start !== 0) {
      parts.push(bodyBuffer.slice(start, end));
    }
    start = end + boundaryBuffer.length;
    end = bodyBuffer.indexOf(boundaryBuffer, start);
  }

  for (const part of parts) {
    if (part.length === 0) continue;

    // Find header/body separator (\r\n\r\n)
    const separatorIndex = part.indexOf(Buffer.from('\r\n\r\n', 'binary'));
    if (separatorIndex === -1) continue;

    const headersBuffer = part.slice(0, separatorIndex);
    const headers = headersBuffer.toString('utf8');

    // Remove trailing \r\n from body
    let bodyData = part.slice(separatorIndex + 4);
    if (bodyData.length >= 2 && bodyData[bodyData.length - 2] === 0x0d && bodyData[bodyData.length - 1] === 0x0a) {
      bodyData = bodyData.slice(0, -2);
    }

    const nameMatch = headers.match(/name="([^"]+)"/);
    const filenameMatch = headers.match(/filename="([^"]+)"/);

    if (!nameMatch) continue;

    const name = nameMatch[1];

    if (filenameMatch) {
      // File - keep as Buffer for binary data
      result.files[name] = {
        filename: filenameMatch[1],
        data: bodyData,
      };
    } else {
      // Regular field - convert to string
      result.fields[name] = bodyData.toString('utf8').trim();
    }
  }

  return result;
}

/**
 * NFC UID 유효성 검사
 * @param {string} nfcUid - NFC UID (14자 hex)
 * @returns {boolean} 유효성 여부
 */
function isValidNfcUid(nfcUid) {
  return /^[0-9A-Fa-f]{14}$/.test(nfcUid);
}

/**
 * 현재 시간 + offset 계산 (ISO 8601)
 * @param {number} minutesOffset - 분 단위 offset
 * @returns {string} ISO 8601 형식 시간
 */
function getExpiresAt(minutesOffset = 10) {
  const date = new Date();
  date.setMinutes(date.getMinutes() + minutesOffset);
  return date.toISOString();
}

/**
 * 로그 출력 헬퍼
 * @param {string} level - 로그 레벨 (INFO, WARN, ERROR)
 * @param {string} message - 메시지
 * @param {Object} data - 추가 데이터
 */
function log(level, message, data = {}) {
  const logEntry = {
    timestamp: new Date().toISOString(),
    level,
    message,
    ...data,
  };
  console.log(JSON.stringify(logEntry));
}

module.exports = {
  generateUserId,
  generateFaceId,
  generateHashKey,
  generateNonce,
  generateNfcUid,
  generateSessionId,
  createResponse,
  createErrorResponse,
  parseMultipartFormData,
  isValidNfcUid,
  getExpiresAt,
  log,
};
