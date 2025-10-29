const db = require('/opt/nodejs/shared/db');
const { createResponse, createErrorResponse, log } = require('/opt/nodejs/shared/utils');

/**
 * HYPERMOB Vehicles Lambda
 * Vehicle list retrieval and management
 */
exports.handler = async (event) => {
  log('INFO', 'Vehicles Lambda invoked', {
    httpMethod: event.httpMethod,
    path: event.path
  });

  try {

    // GET /vehicles - Get vehicle list
    if (event.httpMethod === 'GET' && event.path === '/vehicles') {
      return await getVehicles(event);
    }

    // GET /vehicles/{car_id} - Get specific vehicle
    if (event.httpMethod === 'GET' && event.pathParameters?.car_id) {
      return await getVehicleById(event.pathParameters.car_id);
    }

    // Route not found
    log('WARN', 'Route not found', { httpMethod: event.httpMethod, path: event.path });
    return createErrorResponse(404, 'not_found', 'Route not found');

  } catch (error) {
    log('ERROR', 'Vehicles Lambda error', { error: error.message, stack: error.stack });
    return createErrorResponse(500, 'internal_server_error', 'Internal server error', {
      error: error.message
    });
  }
};

/**
 * Get vehicle list
 */
async function getVehicles(event) {
  const queryParams = event.queryStringParameters || {};
  const status = queryParams.status || 'available';
  const location = queryParams.location;

  log('INFO', 'Fetching vehicles', { status, location });

  let query = 'SELECT * FROM cars WHERE status = ?';
  const params = [status];

  if (location) {
    query += ' AND location LIKE ?';
    params.push(`%${location}%`);
  }

  query += ' ORDER BY created_at DESC';

  const rows = await db.query(query, params);

  log('INFO', 'Vehicles fetched', { count: rows.length });

  // Parse JSON fields
  const vehicles = rows.map(vehicle => ({
    ...vehicle,
    features: typeof vehicle.features === 'string'
      ? JSON.parse(vehicle.features)
      : vehicle.features
  }));

  return createResponse(200, {
    status: 'success',
    data: {
      vehicles: vehicles,
      count: vehicles.length
    }
  });
}

/**
 * Get vehicle by ID
 */
async function getVehicleById(carId) {
  log('INFO', 'Fetching vehicle by ID', { carId });

  const rows = await db.query(
    'SELECT * FROM cars WHERE car_id = ?',
    [carId]
  );

  if (rows.length === 0) {
    log('WARN', 'Vehicle not found', { carId });
    return createErrorResponse(404, 'vehicle_not_found', 'Vehicle not found');
  }

  const vehicle = rows[0];

  // Parse JSON fields
  vehicle.features = typeof vehicle.features === 'string'
    ? JSON.parse(vehicle.features)
    : vehicle.features;

  log('INFO', 'Vehicle fetched successfully', { carId });

  return createResponse(200, {
    status: 'success',
    data: vehicle
  });
}
