/**
 * Lambda: POST /auth/result
 *
 * Report car authentication result
 * - Retrieve auth_sessions by session_id
 * - Update face_verified, ble_verified, nfc_verified
 * - If all 3 are true -> MFA_SUCCESS
 * - If any is false -> MFA_FAILED
 */

const db = require('/opt/nodejs/shared/db');
const {
  createResponse,
  createErrorResponse,
  log,
} = require('/opt/nodejs/shared/utils');

exports.handler = async (event) => {
  log('INFO', 'POST /auth/result - Auth result report started');

  try {
    // 1. Parse request body
    const body = JSON.parse(event.body);
    const { session_id, car_id, face_verified, ble_verified, nfc_verified } = body;

    // 2. Validate input
    if (!session_id || !car_id) {
      return createErrorResponse(400, 'missing_fields', 'session_id and car_id are required');
    }

    if (
      typeof face_verified !== 'boolean' ||
      typeof ble_verified !== 'boolean' ||
      typeof nfc_verified !== 'boolean'
    ) {
      return createErrorResponse(
        400,
        'invalid_fields',
        'face_verified, ble_verified, nfc_verified must be boolean'
      );
    }

    log('INFO', 'Auth result report', {
      session_id,
      car_id,
      face_verified,
      ble_verified,
      nfc_verified,
    });

    // 3. Check if session exists
    const session = await db.queryOne(
      'SELECT session_id, status FROM auth_sessions WHERE session_id = ? AND car_id = ?',
      [session_id, car_id]
    );

    if (!session) {
      return createErrorResponse(404, 'session_not_found', 'Session not found');
    }

    // 4. Determine auth status
    const allVerified = face_verified && ble_verified && nfc_verified;
    const status = allVerified ? 'completed' : 'failed';

    // List of failed steps
    const failedSteps = [];
    if (!face_verified) failedSteps.push('face');
    if (!ble_verified) failedSteps.push('ble');
    if (!nfc_verified) failedSteps.push('nfc');

    log('INFO', 'Auth status determined', { status, failedSteps });

    // 5. Update auth_sessions
    await db.query(
      `UPDATE auth_sessions
       SET face_verified = ?,
           ble_verified = ?,
           nfc_verified = ?,
           status = ?,
           updated_at = NOW()
       WHERE session_id = ?`,
      [face_verified, ble_verified, nfc_verified, status, session_id]
    );

    // 6. Success response
    const response = {
      status: allVerified ? 'MFA_SUCCESS' : 'MFA_FAILED',
      message: allVerified ? 'All auth steps completed' : 'Some auth steps failed',
      session_id,
      timestamp: new Date().toISOString(),
    };

    if (!allVerified) {
      response.failed_steps = failedSteps;
    }

    log('INFO', 'Auth result updated', response);
    return createResponse(200, response);
  } catch (error) {
    log('ERROR', 'Auth result error', { error: error.message, stack: error.stack });
    return createErrorResponse(500, 'internal_error', 'Internal server error', {
      error: error.message,
    });
  }
};
