/**
 * Lambda: POST /users
 *
 * User registration
 * - Create user basic information
 * - Generate and return user_id
 */

const db = require('/opt/nodejs/shared/db');
const { createResponse, createErrorResponse, log } = require('/opt/nodejs/shared/utils');
const crypto = require('crypto');

/**
 * Generate user_id (16 char hex)
 * Format: U + 15 random hex chars = 16 chars
 */
function generateUserId() {
  const randomBytes = crypto.randomBytes(8); // 8 bytes = 16 hex chars
  const hex = randomBytes.toString('hex').toUpperCase();
  return 'U' + hex.substring(0, 15);
}

exports.handler = async (event) => {
  log('INFO', 'POST /users - User registration started');

  try {
    // 1. Parse request body
    const body = JSON.parse(event.body || '{}');
    const { name, email, phone, address } = body;

    // 2. Validation
    if (!name || !email || !phone) {
      return createErrorResponse(400, 'missing_fields', 'name, email, phone are required');
    }

    // Validate email format
    const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
    if (!emailRegex.test(email)) {
      return createErrorResponse(400, 'invalid_email', 'Invalid email format');
    }

    // Validate phone format (Korean phone number)
    const phoneRegex = /^01[0-9]-?[0-9]{3,4}-?[0-9]{4}$/;
    if (!phoneRegex.test(phone)) {
      return createErrorResponse(400, 'invalid_phone', 'Invalid phone format (010-1234-5678)');
    }

    log('INFO', 'User data validated', { name, email, phone });

    // 3. Check if email already exists
    const existingUsers = await db.query(
      'SELECT user_id FROM users WHERE email = ?',
      [email]
    );

    if (existingUsers.length > 0) {
      log('WARN', 'Email already exists', { email });
      return createErrorResponse(409, 'email_exists', 'Email already registered');
    }

    // 4. Generate user_id
    const userId = generateUserId();
    log('INFO', 'Generated user_id', { userId });

    // 5. Insert into database
    await db.query(
      `INSERT INTO users (user_id, name, email, phone, address, created_at, updated_at)
       VALUES (?, ?, ?, ?, ?, NOW(), NOW())`,
      [userId, name, email, phone, address || null]
    );

    log('INFO', 'User created successfully', { userId, email });

    // 6. Success response
    const response = {
      user_id: userId,
      name: name,
      email: email,
      status: 'created'
    };

    return createResponse(201, response);

  } catch (error) {
    log('ERROR', 'User registration error', { error: error.message, stack: error.stack });
    return createErrorResponse(500, 'internal_error', 'Internal server error', {
      error: error.message,
    });
  }
};
