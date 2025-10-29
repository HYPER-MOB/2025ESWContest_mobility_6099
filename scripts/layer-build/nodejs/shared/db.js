/**
 * Common database connection module
 *
 * MySQL RDS connection management for AWS Lambda
 * - Uses connection pool (improves performance on Lambda reuse)
 * - Manages DB settings via environment variables
 */

const mysql = require('mysql2/promise');

// Connection pool (improves performance on Lambda container reuse)
let pool = null;

/**
 * Create or reuse MySQL connection pool
 */
function getPool() {
  if (!pool) {
    pool = mysql.createPool({
      host: process.env.DB_HOST,
      port: process.env.DB_PORT || 3306,
      user: process.env.DB_USER,
      password: process.env.DB_PASSWORD,
      database: process.env.DB_NAME || 'hypermob',
      waitForConnections: true,
      connectionLimit: 5, // Consider Lambda concurrency limits
      queueLimit: 0,
      enableKeepAlive: true,
      keepAliveInitialDelay: 0,
      timezone: '+00:00', // UTC
    });

    console.log('MySQL connection pool created');
  }

  return pool;
}

/**
 * Get database connection
 */
async function getConnection() {
  const pool = getPool();
  return await pool.getConnection();
}

/**
 * Query execution helper function
 * @param {string} sql - SQL query
 * @param {Array} params - Parameter array
 * @returns {Promise<Array>} Query results
 */
async function query(sql, params = []) {
  const connection = await getConnection();
  try {
    const [rows] = await connection.execute(sql, params);
    return rows;
  } finally {
    connection.release();
  }
}

/**
 * Query single row
 * @param {string} sql - SQL query
 * @param {Array} params - Parameter array
 * @returns {Promise<Object|null>} Single row or null
 */
async function queryOne(sql, params = []) {
  const rows = await query(sql, params);
  return rows.length > 0 ? rows[0] : null;
}

/**
 * Execute transaction
 * @param {Function} callback - Transaction logic (receives connection as argument)
 * @returns {Promise<any>} Transaction result
 */
async function transaction(callback) {
  const connection = await getConnection();
  try {
    await connection.beginTransaction();
    const result = await callback(connection);
    await connection.commit();
    return result;
  } catch (error) {
    await connection.rollback();
    throw error;
  } finally {
    connection.release();
  }
}

/**
 * Database health check
 * @returns {Promise<boolean>} Connection success status
 */
async function healthCheck() {
  try {
    const result = await queryOne('SELECT 1 as health');
    return result && result.health === 1;
  } catch (error) {
    console.error('Database health check failed:', error);
    return false;
  }
}

module.exports = {
  getPool,
  getConnection,
  query,
  queryOne,
  transaction,
  healthCheck,
};
