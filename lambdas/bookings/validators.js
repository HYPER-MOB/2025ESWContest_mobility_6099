/**
 * Booking request validation
 */
function validateBooking(body) {
  const { user_id, car_id, start_time, end_time } = body;

  // Check required fields
  if (!user_id || !car_id || !start_time || !end_time) {
    return {
      valid: false,
      message: 'Missing required fields (user_id, car_id, start_time, end_time)'
    };
  }

  // Validate time format
  const start = new Date(start_time);
  const end = new Date(end_time);

  if (isNaN(start.getTime()) || isNaN(end.getTime())) {
    return {
      valid: false,
      message: 'Invalid time format (ISO 8601 format required)'
    };
  }

  // Check end time is after start time
  if (end <= start) {
    return {
      valid: false,
      message: 'End time must be after start time'
    };
  }

  // Prevent past time bookings
  const now = new Date();
  if (start < now) {
    return {
      valid: false,
      message: 'Cannot book past time'
    };
  }

  // Booking duration limit (max 7 days)
  const maxDuration = 7 * 24 * 60 * 60 * 1000; // 7 days
  if (end - start > maxDuration) {
    return {
      valid: false,
      message: 'Booking period is limited to 7 days maximum'
    };
  }

  return { valid: true };
}

module.exports = { validateBooking };
