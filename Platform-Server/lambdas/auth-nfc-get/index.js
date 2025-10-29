/**
 * Lambda: GET /auth/nfc/uid
 *
 * NFC UID retrieval
 * - Retrieve server-generated nfc_uid by user_id
 * - App fetches UID to use for NFC HCE
 */

const db = require('/opt/nodejs/shared/db');
const {
  createResponse,
  createErrorResponse,
  log,
} = require('/opt/nodejs/shared/utils');

exports.handler = async (event) => {
  log('INFO', 'GET /auth/nfc/uid - NFC UID retrieval started');

  try {
    // 1. Parse query parameters
    const userId = event.queryStringParameters?.user_id;

    if (!userId) {
      return createErrorResponse(400, 'missing_user_id', 'user_id is required');
    }

    log('INFO', 'Query parameters', { userId });

    // 2. Retrieve user from database
    const rows = await db.query(
      'SELECT user_id, nfc_uid FROM users WHERE user_id = ?',
      [userId]
    );

    if (rows.length === 0) {
      log('WARN', 'User not found', { userId });
      return createErrorResponse(404, 'user_not_found', 'User not found');
    }

    const user = rows[0];

    // 3. Success response
    const response = {
      user_id: user.user_id,
      nfc_uid: user.nfc_uid,
      status: 'ok',
    };

    log('INFO', 'NFC UID retrieval successful', response);
    return createResponse(200, response);
  } catch (error) {
    log('ERROR', 'NFC UID retrieval error', { error: error.message, stack: error.stack });
    return createErrorResponse(500, 'internal_error', 'Internal server error', {
      error: error.message,
    });
  }
};
