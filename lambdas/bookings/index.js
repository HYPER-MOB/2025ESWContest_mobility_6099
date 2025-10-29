const db = require('/opt/nodejs/shared/db');
const { createResponse, createErrorResponse, log } = require('/opt/nodejs/shared/utils');
const { validateBooking } = require('./validators');

/**
 * HYPERMOB Bookings Lambda
 * Vehicle booking creation and management
 */
exports.handler = async (event) => {
  log('INFO', 'Bookings Lambda invoked', {
    httpMethod: event.httpMethod,
    path: event.path
  });

  try {
    // POST /bookings - Create booking
    if (event.httpMethod === 'POST' && event.path === '/bookings') {
      return await createBooking(event);
    }

    // GET /bookings/{booking_id} - Get booking
    if (event.httpMethod === 'GET' && event.pathParameters?.booking_id) {
      return await getBooking(event.pathParameters.booking_id);
    }

    // PATCH /bookings/{booking_id} - Update/cancel booking
    if (event.httpMethod === 'PATCH' && event.pathParameters?.booking_id) {
      return await updateBooking(event);
    }

    // GET /bookings - Get user bookings
    if (event.httpMethod === 'GET' && event.path === '/bookings') {
      return await getBookings(event);
    }

    // Route not found
    log('WARN', 'Route not found', { httpMethod: event.httpMethod, path: event.path });
    return createErrorResponse(404, 'not_found', 'Route not found');

  } catch (error) {
    log('ERROR', 'Bookings Lambda error', { error: error.message, stack: error.stack });
    return createErrorResponse(500, 'internal_server_error', 'Internal server error', {
      error: error.message
    });
  }
};

/**
 * Create booking
 */
async function createBooking(event) {
  const body = JSON.parse(event.body || '{}');
  const { user_id, car_id, start_time, end_time } = body;

  // Validate input
  const validation = validateBooking(body);
  if (!validation.valid) {
    return createErrorResponse(400, 'invalid_request', validation.message);
  }

  log('INFO', 'Creating booking', { user_id, car_id, start_time, end_time });

  // Use transaction for atomic operation
  return await db.transaction(async (connection) => {
    // Check vehicle exists and is available
    const [vehicles] = await connection.execute(
      'SELECT * FROM cars WHERE car_id = ? FOR UPDATE',
      [car_id]
    );

    if (vehicles.length === 0) {
      log('WARN', 'Vehicle not found', { car_id });
      return createErrorResponse(404, 'vehicle_not_found', 'Vehicle not found');
    }

    if (vehicles[0].status !== 'available') {
      log('WARN', 'Vehicle not available', { car_id, status: vehicles[0].status });
      return createErrorResponse(409, 'vehicle_not_available', 'Vehicle not available', {
        current_status: vehicles[0].status
      });
    }

    // Check for booking conflicts
    const [conflicts] = await connection.execute(
      `SELECT * FROM bookings
       WHERE car_id = ?
       AND status IN ('approved', 'active')
       AND (
         (start_time <= ? AND end_time > ?)
         OR (start_time < ? AND end_time >= ?)
         OR (start_time >= ? AND end_time <= ?)
       )`,
      [car_id, start_time, start_time, end_time, end_time, start_time, end_time]
    );

    if (conflicts.length > 0) {
      log('WARN', 'Booking conflict detected', { car_id, conflicts: conflicts.length });
      return createErrorResponse(409, 'booking_conflict', 'Booking conflict exists', {
        conflicting_bookings: conflicts.map(b => ({
          booking_id: b.booking_id,
          start_time: b.start_time,
          end_time: b.end_time
        }))
      });
    }

    // Create booking
    const booking_id = `B${Date.now().toString().slice(-15)}`;
    await connection.execute(
      `INSERT INTO bookings
       (booking_id, user_id, car_id, status, start_time, end_time)
       VALUES (?, ?, ?, 'approved', ?, ?)`,
      [booking_id, user_id, car_id, start_time, end_time]
    );

    // Update vehicle status
    await connection.execute(
      'UPDATE cars SET status = ?, updated_at = CURRENT_TIMESTAMP WHERE car_id = ?',
      ['rented', car_id]
    );

    log('INFO', 'Booking created successfully', { booking_id });

    return createResponse(201, {
      status: 'success',
      data: {
        booking_id,
        user_id,
        car_id,
        status: 'approved',
        start_time,
        end_time,
        created_at: new Date().toISOString()
      }
    });
  });
}

/**
 * Get booking by ID
 */
async function getBooking(bookingId) {
  log('INFO', 'Fetching booking', { bookingId });

  const rows = await db.query(
    'SELECT * FROM bookings WHERE booking_id = ?',
    [bookingId]
  );

  if (rows.length === 0) {
    log('WARN', 'Booking not found', { bookingId });
    return createErrorResponse(404, 'booking_not_found', 'Booking not found');
  }

  log('INFO', 'Booking fetched successfully', { bookingId });
  return createResponse(200, rows[0]);
}

/**
 * Update booking (cancel)
 */
async function updateBooking(event) {
  const bookingId = event.pathParameters.booking_id;
  const body = JSON.parse(event.body || '{}');
  const { status } = body;

  if (!status || !['cancelled'].includes(status)) {
    return createErrorResponse(400, 'invalid_status', 'Invalid status (only "cancelled" allowed)');
  }

  log('INFO', 'Updating booking', { bookingId, status });

  return await db.transaction(async (connection) => {
    // Get booking
    const [bookings] = await connection.execute(
      'SELECT * FROM bookings WHERE booking_id = ? FOR UPDATE',
      [bookingId]
    );

    if (bookings.length === 0) {
      log('WARN', 'Booking not found', { bookingId });
      return createErrorResponse(404, 'booking_not_found', 'Booking not found');
    }

    const booking = bookings[0];

    // Update booking status
    await connection.execute(
      'UPDATE bookings SET status = ?, updated_at = CURRENT_TIMESTAMP WHERE booking_id = ?',
      [status, bookingId]
    );

    // Update vehicle status to available
    if (status === 'cancelled') {
      await connection.execute(
        'UPDATE cars SET status = ?, updated_at = CURRENT_TIMESTAMP WHERE car_id = ?',
        ['available', booking.car_id]
      );
    }

    log('INFO', 'Booking updated successfully', { bookingId, status });

    return createResponse(200, {
      booking_id: bookingId,
      status: status,
      updated_at: new Date().toISOString()
    });
  });
}

/**
 * Get user bookings
 */
async function getBookings(event) {
  const queryParams = event.queryStringParameters || {};
  const userId = queryParams.user_id;

  if (!userId) {
    return createErrorResponse(400, 'missing_parameter', 'user_id parameter is required');
  }

  log('INFO', 'Fetching user bookings', { userId });

  const rows = await db.query(
    'SELECT * FROM bookings WHERE user_id = ? ORDER BY created_at DESC',
    [userId]
  );

  log('INFO', 'User bookings fetched', { userId, count: rows.length });

  return createResponse(200, {
    status: 'success',
    data: {
      bookings: rows,
      count: rows.length
    }
  });
}
