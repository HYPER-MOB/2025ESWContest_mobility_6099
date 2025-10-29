const db = require('/opt/nodejs/shared/db');
const { createResponse, createErrorResponse, log } = require('/opt/nodejs/shared/utils');
const { applyVehicleSettings } = require('./vehicle-interface');

/**
 * HYPERMOB Vehicle Control Lambda
 * Vehicle settings application and control
 */
exports.handler = async (event) => {
  log('INFO', 'Vehicle Control Lambda invoked', {
    httpMethod: event.httpMethod,
    path: event.path
  });

  try {
    const carId = event.pathParameters?.car_id;

    // POST /vehicles/{car_id}/settings/apply - Auto apply profile
    if (event.httpMethod === 'POST' && event.path.includes('/settings/apply')) {
      return await applySettings(event, carId);
    }

    // POST /vehicles/{car_id}/settings/manual - Record manual adjustment
    if (event.httpMethod === 'POST' && event.path.includes('/settings/manual')) {
      return await recordManualSettings(event, carId);
    }

    // GET /vehicles/{car_id}/settings/current - Get current settings
    if (event.httpMethod === 'GET' && event.path.includes('/settings/current')) {
      return await getCurrentSettings(carId);
    }

    // GET /vehicles/{car_id}/settings/history - Get settings history
    if (event.httpMethod === 'GET' && event.path.includes('/settings/history')) {
      return await getSettingsHistory(event, carId);
    }

    // Route not found
    log('WARN', 'Route not found', { httpMethod: event.httpMethod, path: event.path });
    return createErrorResponse(404, 'not_found', 'Route not found');

  } catch (error) {
    log('ERROR', 'Vehicle Control Lambda error', { error: error.message, stack: error.stack });
    return createErrorResponse(500, 'internal_server_error', 'Internal server error', {
      error: error.message
    });
  }
};

/**
 * Auto apply vehicle settings
 */
async function applySettings(event, carId) {
  const body = JSON.parse(event.body || '{}');
  const { user_id, settings } = body;

  if (!user_id || !settings) {
    return createErrorResponse(400, 'invalid_request', 'user_id and settings are required');
  }

  log('INFO', 'Applying vehicle settings', { carId, user_id });

  // Check vehicle exists
  const vehicles = await db.query(
    'SELECT * FROM cars WHERE car_id = ?',
    [carId]
  );

  if (vehicles.length === 0) {
    log('WARN', 'Vehicle not found', { carId });
    return createErrorResponse(404, 'vehicle_not_found', 'Vehicle not found');
  }

  try {
    // Apply vehicle settings (mock implementation or actual CAN communication)
    const result = await applyVehicleSettings(carId, settings);

    // Save to vehicle_settings table
    await db.query(
      `INSERT INTO vehicle_settings
       (car_id, user_id, seat_position, mirror_position, steering_position, climate_settings)
       VALUES (?, ?, ?, ?, ?, ?)`,
      [
        carId,
        user_id,
        JSON.stringify(settings.seat_position || {}),
        JSON.stringify(settings.mirror_position || {}),
        JSON.stringify(settings.steering_position || {}),
        JSON.stringify(settings.climate_settings || {})
      ]
    );
    log('INFO', 'Settings saved to vehicle_settings');

    log('INFO', 'Vehicle settings applied successfully', { carId, user_id });

    return createResponse(200, {
      status: 'success',
      data: {
        car_id: carId,
        applied_settings: result,
        timestamp: new Date().toISOString()
      }
    });

  } catch (applyError) {
    log('ERROR', 'Settings apply error', { error: applyError.message });
    return createErrorResponse(500, 'apply_failed', 'Failed to apply settings', {
      error: applyError.message
    });
  }
}

/**
 * Record manual adjustment
 */
async function recordManualSettings(event, carId) {
  const body = JSON.parse(event.body || '{}');
  const { user_id, settings } = body;

  if (!user_id || !settings) {
    return createErrorResponse(400, 'invalid_request', 'user_id and settings are required');
  }

  log('INFO', 'Recording manual settings', { carId, user_id });

  // Check vehicle exists
  const vehicles = await db.query(
    'SELECT car_id FROM cars WHERE car_id = ?',
    [carId]
  );

  if (vehicles.length === 0) {
    log('WARN', 'Vehicle not found', { carId });
    return createErrorResponse(404, 'vehicle_not_found', 'Vehicle not found');
  }

  // Record manual adjustment in vehicle_settings table
  await db.query(
    `INSERT INTO vehicle_settings
     (car_id, user_id, seat_position, mirror_position, steering_position, climate_settings)
     VALUES (?, ?, ?, ?, ?, ?)`,
    [
      carId,
      user_id,
      JSON.stringify(settings.seat_position || {}),
      JSON.stringify(settings.mirror_position || {}),
      JSON.stringify(settings.steering_position || {}),
      JSON.stringify(settings.climate_settings || {})
    ]
  );

  log('INFO', 'Manual settings recorded', { carId, user_id });

  return createResponse(200, {
    message: 'Manual adjustment recorded',
    car_id: carId,
    user_id: user_id,
    timestamp: new Date().toISOString()
  });
}

/**
 * Get current settings
 */
async function getCurrentSettings(carId) {
  log('INFO', 'Fetching current settings', { carId });

  // Get most recent applied settings
  const rows = await db.query(
    `SELECT * FROM vehicle_settings
     WHERE car_id = ?
     ORDER BY created_at DESC
     LIMIT 1`,
    [carId]
  );

  if (rows.length === 0) {
    log('WARN', 'No settings found', { carId });
    return createErrorResponse(404, 'no_settings_found', 'No settings records found');
  }

  const record = rows[0];

  log('INFO', 'Current settings fetched', { carId });

  return createResponse(200, {
    car_id: carId,
    user_id: record.user_id,
    seat_position: typeof record.seat_position === 'string' ? JSON.parse(record.seat_position) : record.seat_position,
    mirror_position: typeof record.mirror_position === 'string' ? JSON.parse(record.mirror_position) : record.mirror_position,
    steering_position: typeof record.steering_position === 'string' ? JSON.parse(record.steering_position) : record.steering_position,
    climate_settings: typeof record.climate_settings === 'string' ? JSON.parse(record.climate_settings) : record.climate_settings,
    created_at: record.created_at
  });
}

/**
 * Get settings history
 */
async function getSettingsHistory(event, carId) {
  const queryParams = event.queryStringParameters || {};
  const userId = queryParams.user_id;
  const limit = parseInt(queryParams.limit) || 10;

  log('INFO', 'Fetching settings history', { carId, userId, limit });

  let query = 'SELECT * FROM vehicle_settings WHERE car_id = ?';
  const params = [carId];

  if (userId) {
    query += ' AND user_id = ?';
    params.push(userId);
  }

  query += ' ORDER BY created_at DESC LIMIT ?';
  params.push(limit);

  const rows = await db.query(query, params);

  // Parse JSON fields
  const history = rows.map(record => ({
    setting_id: record.setting_id,
    user_id: record.user_id,
    car_id: record.car_id,
    seat_position: typeof record.seat_position === 'string' ? JSON.parse(record.seat_position) : record.seat_position,
    mirror_position: typeof record.mirror_position === 'string' ? JSON.parse(record.mirror_position) : record.mirror_position,
    steering_position: typeof record.steering_position === 'string' ? JSON.parse(record.steering_position) : record.steering_position,
    climate_settings: typeof record.climate_settings === 'string' ? JSON.parse(record.climate_settings) : record.climate_settings,
    created_at: record.created_at
  }));

  log('INFO', 'Settings history fetched', { carId, count: history.length });

  return createResponse(200, {
    car_id: carId,
    history: history,
    count: history.length
  });
}
