/**
 * Lambda: POST /auth/nfc/verify
 *
 * Final NFC authentication verification (for App)
 * - Retrieve auth_sessions by user_id and car_id
 * - Verify NFC UID matches
 * - Check face_verified and ble_verified are both true
 * - If all pass -> nfc_verified = true, status = MFA_SUCCESS
 */

const db = require('/opt/nodejs/shared/db');
const {
  createResponse,
  createErrorResponse,
  isValidNfcUid,
  log,
} = require('/opt/nodejs/shared/utils');

exports.handler = async (event) => {
  log('INFO', 'POST /auth/nfc/verify - NFC verification started');

  try {
    // 1. Parse request body
    const body = JSON.parse(event.body);
    const { user_id, nfc_uid, car_id } = body;

    // 2. Validate input
    if (!user_id || !nfc_uid || !car_id) {
      return createErrorResponse(400, 'missing_fields', 'user_id, nfc_uid, and car_id are required');
    }

    if (!isValidNfcUid(nfc_uid)) {
      return createErrorResponse(400, 'invalid_nfc_uid', 'NFC UID must be 14 hex characters');
    }

    log('INFO', 'NFC verification request', { user_id, nfc_uid, car_id });

    // 3. Get user's NFC UID from users table
    const user = await db.queryOne(
      'SELECT nfc_uid FROM users WHERE user_id = ?',
      [user_id]
    );

    if (!user) {
      return createErrorResponse(404, 'user_not_found', 'User not found');
    }

    // 4. Verify NFC UID matches
    if (user.nfc_uid !== nfc_uid) {
      log('WARN', 'NFC UID mismatch', {
        expected: user.nfc_uid,
        received: nfc_uid,
      });
      return createErrorResponse(400, 'nfc_uid_mismatch', 'NFC UID does not match');
    }

    // 5. Retrieve active auth session (non-expired, most recent)
    const session = await db.queryOne(
      `SELECT session_id, face_verified, ble_verified, nfc_verified, status
       FROM auth_sessions
       WHERE user_id = ? AND car_id = ? AND expires_at > NOW()
       ORDER BY created_at DESC
       LIMIT 1`,
      [user_id, car_id]
    );

    if (!session) {
      return createErrorResponse(
        404,
        'session_not_found',
        'No active session found for this user_id and car_id'
      );
    }

    // 6. Check previous steps are complete
    if (!session.face_verified || !session.ble_verified) {
      const completedSteps = [];
      if (session.face_verified) completedSteps.push('face');
      if (session.ble_verified) completedSteps.push('ble');

      log('WARN', 'Previous steps incomplete', { completedSteps });

      return createErrorResponse(
        400,
        'previous_steps_incomplete',
        'Face or BLE authentication is not yet completed',
        { completed_steps: completedSteps }
      );
    }

    // 7. Complete NFC verification
    await db.query(
      `UPDATE auth_sessions
       SET nfc_verified = TRUE,
           status = 'completed',
           updated_at = NOW()
       WHERE session_id = ?`,
      [session.session_id]
    );

    // 8. Success response
    const response = {
      status: 'completed',
      message: 'All auth steps completed',
      session_id: session.session_id,
      timestamp: new Date().toISOString(),
    };

    log('INFO', 'NFC verification successful', response);
    return createResponse(200, response);
  } catch (error) {
    log('ERROR', 'NFC verification error', { error: error.message, stack: error.stack });
    return createErrorResponse(500, 'internal_error', 'Internal server error', {
      error: error.message,
    });
  }
};
