/**
 * Lambda: POST /auth/nfc
 *
 * NFC UID registration
 * - Validate user_id
 * - Validate NFC UID format (14 hex chars)
 * - Update NFC UID in DB
 */

const db = require('/opt/nodejs/shared/db');
const {
  createResponse,
  createErrorResponse,
  isValidNfcUid,
  log,
} = require('/opt/nodejs/shared/utils');

exports.handler = async (event) => {
  log('INFO', 'POST /auth/nfc - NFC registration started');

  try {
    // 1. Parse request body
    const body = JSON.parse(event.body);
    const { user_id, nfc_uid } = body;

    // 2. Validate input
    if (!user_id || !nfc_uid) {
      return createErrorResponse(400, 'missing_fields', 'user_id and nfc_uid are required');
    }

    if (!isValidNfcUid(nfc_uid)) {
      return createErrorResponse(400, 'invalid_nfc_uid', 'NFC UID must be 14 hex characters');
    }

    log('INFO', 'NFC registration request', { user_id, nfc_uid });

    // 3. Check if user exists
    const user = await db.queryOne('SELECT user_id FROM users WHERE user_id = ?', [user_id]);

    if (!user) {
      return createErrorResponse(404, 'user_not_found', 'User not found');
    }

    // 4. Update NFC UID
    try {
      await db.query('UPDATE users SET nfc_uid = ? WHERE user_id = ?', [nfc_uid, user_id]);
    } catch (dbError) {
      // Unique constraint violation (duplicate nfc_uid)
      if (dbError.code === 'ER_DUP_ENTRY') {
        log('WARN', 'Duplicate nfc_uid', { nfc_uid });
        return createErrorResponse(409, 'nfc_uid_already_exists', 'This NFC UID is already registered');
      }
      throw dbError;
    }

    // 5. Success response
    const response = {
      status: 'ok',
    };

    log('INFO', 'NFC registration successful', { user_id, nfc_uid });
    return createResponse(200, response);
  } catch (error) {
    log('ERROR', 'NFC registration error', { error: error.message, stack: error.stack });
    return createErrorResponse(500, 'internal_error', 'Internal server error', {
      error: error.message,
    });
  }
};
