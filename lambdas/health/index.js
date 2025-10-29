/**
 * Lambda: GET /health
 *
 * ?�버 ?�스 체크
 * - API ?�버 ?�태 ?�인
 * - ?�이?�베?�스 ?�결 ?�태 ?�인
 */

const db = require('/opt/nodejs/shared/db');
const { createResponse, createErrorResponse, log } = require('/opt/nodejs/shared/utils');

exports.handler = async (event) => {
  log('INFO', 'Health check started');

  try {
    // ?�이?�베?�스 ?�결 ?�스??
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
