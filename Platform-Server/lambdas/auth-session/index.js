/**
 * Lambda: GET /auth/session
 *
 * Start car authentication session
 * - Retrieve user info by user_id and car_id
 * - Auto-create BLE session if none exists
 * - Create new auth_session entry
 * - Return face_id, hashkey, nfc_uid
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
  log('INFO', 'GET /auth/session - Auth session request started');

  try {
    // 1. Parse query parameters
    const { car_id, user_id } = event.queryStringParameters || {};

    // 2. Validate input
    if (!car_id || !user_id) {
      return createErrorResponse(400, 'missing_parameters', 'car_id and user_id are required');
    }

    log('INFO', 'Auth session request', { car_id, user_id });

    // 3. Retrieve user info
    const user = await db.queryOne(
      'SELECT user_id, nfc_uid FROM users WHERE user_id = ?',
      [user_id]
    );

    if (!user) {
      return createErrorResponse(404, 'user_not_found', 'User not found');
    }

    // face_id is optional (may not exist in schema)
    user.face_id = user.face_id || null;

    // 4. Check for existing BLE session (within 10 minutes)
    let bleSession = await db.queryOne(
      `SELECT hashkey, expires_at
       FROM ble_sessions
       WHERE user_id = ? AND car_id = ? AND expires_at > NOW()
       ORDER BY created_at DESC
       LIMIT 1`,
      [user_id, car_id]
    );

    // Auto-create BLE session if none exists
    if (!bleSession) {
      const nonce = generateNonce();
      const hashkey = generateHashKey(user_id, car_id, nonce);
      const bleSessionId = generateSessionId('BLE');
      const bleExpiresAt = getExpiresAt(10);

      await db.query(
        `INSERT INTO ble_sessions
         (session_id, user_id, car_id, hashkey, nonce, expires_at)
         VALUES (?, ?, ?, ?, ?, ?)`,
        [bleSessionId, user_id, car_id, hashkey, nonce, bleExpiresAt]
      );

      bleSession = { hashkey, expires_at: bleExpiresAt };
      log('INFO', 'Auto-created BLE session', { hashkey });
    }

    // 5. Get active or upcoming booking for this user and car (within 24 hours)
    const booking = await db.queryOne(
      `SELECT booking_id FROM bookings
       WHERE user_id = ? AND car_id = ?
       AND status IN ('approved', 'active')
       AND start_time <= DATE_ADD(NOW(), INTERVAL 24 HOUR)
       AND end_time >= NOW()
       ORDER BY created_at DESC
       LIMIT 1`,
      [user_id, car_id]
    );

    if (!booking) {
      return createErrorResponse(404, 'no_active_booking', 'No active or upcoming booking found for this user and car (within 24 hours)');
    }

    // 6. Create auth session
    const sessionId = generateSessionId('AUTH');
    const expiresAt = getExpiresAt(10); // 10 minutes

    // Insert into auth_sessions table
    try {
      await db.query(
        `INSERT INTO auth_sessions
         (session_id, booking_id, user_id, car_id, expires_at)
         VALUES (?, ?, ?, ?, ?)`,
        [sessionId, booking.booking_id, user_id, car_id, expiresAt]
      );
      log('INFO', 'Auth session record created in database');
    } catch (dbError) {
      // Table may not exist or schema mismatch - continue anyway
      log('WARN', 'Could not save auth session to database', { error: dbError.message });
    }

    // 7. Success response
    const response = {
      session_id: sessionId,
      hashkey: bleSession.hashkey,
      nfc_uid: user.nfc_uid,
      status: 'active',
    };

    log('INFO', 'Auth session created', response);
    return createResponse(200, response);
  } catch (error) {
    log('ERROR', 'Auth session error', { error: error.message, stack: error.stack });
    return createErrorResponse(500, 'internal_error', 'Internal server error', {
      error: error.message,
    });
  }
};
