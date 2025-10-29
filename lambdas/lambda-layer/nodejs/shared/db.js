/**
 * 공통 데이터베이스 연결 모듈
 *
 * AWS Lambda에서 MySQL RDS 연결 관리
 * - 연결 풀 사용 (Lambda 재사용 시 성능 향상)
 * - 환경 변수로 DB 설정 관리
 */

const mysql = require('mysql2/promise');

// 연결 풀 (Lambda 컨테이너 재사용 시 성능 향상)
let pool = null;

/**
 * MySQL 연결 풀 생성 또는 재사용
 */
function getPool() {
  if (!pool) {
    pool = mysql.createPool({
      host: process.env.DB_HOST,
      port: process.env.DB_PORT || 3306,
      user: process.env.DB_USER,
      password: process.env.DB_PASSWORD,
      database: process.env.DB_NAME || 'hypermob_mfa',
      waitForConnections: true,
      connectionLimit: 5, // Lambda의 동시성 제한 고려
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
 * 데이터베이스 연결 가져오기
 */
async function getConnection() {
  const pool = getPool();
  return await pool.getConnection();
}

/**
 * 쿼리 실행 헬퍼 함수
 * @param {string} sql - SQL 쿼리
 * @param {Array} params - 파라미터 배열
 * @returns {Promise<Array>} 쿼리 결과
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
 * 단일 행 조회
 * @param {string} sql - SQL 쿼리
 * @param {Array} params - 파라미터 배열
 * @returns {Promise<Object|null>} 단일 행 또는 null
 */
async function queryOne(sql, params = []) {
  const rows = await query(sql, params);
  return rows.length > 0 ? rows[0] : null;
}

/**
 * 트랜잭션 실행
 * @param {Function} callback - 트랜잭션 내부 로직 (connection을 인자로 받음)
 * @returns {Promise<any>} 트랜잭션 결과
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
 * 데이터베이스 헬스 체크
 * @returns {Promise<boolean>} 연결 성공 여부
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
