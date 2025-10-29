/**
 * Lambda: GET /health
 *
 * ?úÎ≤Ñ ?¨Ïä§ Ï≤¥ÌÅ¨
 * - API ?úÎ≤Ñ ?ÅÌÉú ?ïÏù∏
 * - ?∞Ïù¥?∞Î≤†?¥Ïä§ ?∞Í≤∞ ?ÅÌÉú ?ïÏù∏
 */

const db = require('/opt/nodejs/shared/db');
const { createResponse, createErrorResponse, log } = require('/opt/nodejs/shared/utils');

exports.handler = async (event) => {
  log('INFO', 'Health check started');

  try {
    // ?∞Ïù¥?∞Î≤†?¥Ïä§ ?∞Í≤∞ ?åÏä§??
    const dbConnected = await db.healthCheck();

    const response = {
      status: 'ok',
      timestamp: new Date().toISOString(),
      database: dbConnected ? 'connected' : 'disconnected',
    };

    if (!dbConnected) {
      log('WARN', 'Database connection failed');
      return createResponse(503, response);
    }

    log('INFO', 'Health check passed', response);
    return createResponse(200, response);
  } catch (error) {
    log('ERROR', 'Health check error', { error: error.message });
    return createErrorResponse(503, 'health_check_failed', 'Server is not healthy', {
      error: error.message,
    });
  }
};
