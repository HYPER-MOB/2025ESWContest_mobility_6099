/**
 * Database Configuration
 * 환경 변수에서 DB 연결 정보 로드
 */
module.exports = {
  host: process.env.DB_HOST || 'localhost',
  user: process.env.DB_USER || 'hypermob',
  password: process.env.DB_PASSWORD || '',
  database: process.env.DB_NAME || 'hypermob',
  port: process.env.DB_PORT || 3306,
  charset: 'utf8mb4',
  timezone: '+00:00',
  connectTimeout: 10000,
  // Connection pool 설정 (선택)
  waitForConnections: true,
  connectionLimit: 10,
  queueLimit: 0
};
