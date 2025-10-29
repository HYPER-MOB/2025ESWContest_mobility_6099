/**
 * Lambda: GET /auth/ble
 *
 * BLE hashkey request
 * - Validate user_id
 * - Generate BLE hashkey (SHA256)
 * - Store in ble_sessions table
 * - Expiration: 10 minutes
 */

const db = require('/opt/nodejs/shared/db');
const {
  generateHashKey,
  generateNonce,
  generateSessionId,
  createResponse,
  createErrorResponse,
  getExpiresAt,
  log,
} = require('/opt/nodejs/shared/utils');

exports.handler = async (event) => {
  log('INFO', 'GET /auth/ble - BLE hashkey request started');

  try {
    // 1. Parse query parameters
    const { user_id, car_id = 'CAR123' } = event.queryStringParameters || {};

    // 2. Validate input
    if (!user_id) {
      return createErrorResponse(400, 'missing_user_id', 'user_id is required');
    }

    log('INFO', 'BLE hashkey request', { user_id, car_id });

    // 3. Check if user exists
    const user = await db.queryOne('SELECT user_id FROM users WHERE user_id = ?', [user_id]);

    if (!user) {
      return createErrorResponse(404, 'user_not_found', 'User not found');
    }

    // 4. Generate BLE hashkey
    const nonce = generateNonce();
    const hashkey = generateHashKey(user_id, car_id, nonce);
    const sessionId = generateSessionId('BLE');
    const expiresAt = getExpiresAt(10); // 10 minutes

    log('INFO', 'Generated BLE hashkey', { sessionId, hashkey, expiresAt });

    // 5. Store in ble_sessions table
    await db.query(
      `INSERT INTO ble_sessions
       (session_id, user_id, car_id, hashkey, nonce, expires_at)
       VALUES (?, ?, ?, ?, ?, ?)`,
      [sessionId, user_id, car_id, hashkey, nonce, expiresAt]
    );

    // 6. Success response
    const response = {
      hashkey,
      expires_at: expiresAt,
    };

    log('INFO', 'BLE hashkey issued', response);
    return createResponse(200, response);
  } catch (error) {
    log('ERROR', 'BLE hashkey error', { error: error.message, stack: error.stack });
    return createErrorResponse(500, 'internal_error', 'Internal server error', {
      error: error.message,
    });
  }
};
