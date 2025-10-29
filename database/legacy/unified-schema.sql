-- ============================================================
-- HYPERMOB Unified Database Schema
-- ============================================================
-- 목적: Platform-Application + Platform-AI 통합 데이터베이스
-- DBMS: MySQL 8.0+
-- 인코딩: UTF8MB4
-- 타임존: UTC
-- ============================================================

-- 데이터베이스 생성
CREATE DATABASE IF NOT EXISTS hypermob
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_unicode_ci;

USE hypermob;

-- ============================================================
-- 1. users 테이블 (사용자 기본 정보)
-- ============================================================

DROP TABLE IF EXISTS users;

CREATE TABLE users (
    user_id VARCHAR(16) NOT NULL PRIMARY KEY COMMENT '사용자 고유 ID (U + 15자 hex)',
    name VARCHAR(100) NOT NULL COMMENT '사용자 이름',
    email VARCHAR(255) NOT NULL UNIQUE COMMENT '이메일',
    phone VARCHAR(20) NOT NULL COMMENT '전화번호',
    address VARCHAR(500) COMMENT '주소',
    face_id VARCHAR(16) COMMENT '얼굴 해시 ID',
    nfc_uid VARCHAR(14) COMMENT 'NFC UID (14자 hex)',
    profile_image_url VARCHAR(512) COMMENT '프로필 이미지 URL',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    INDEX idx_email (email),
    INDEX idx_phone (phone),
    UNIQUE KEY uk_face_id (face_id),
    UNIQUE KEY uk_nfc_uid (nfc_uid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='사용자 기본 정보';

-- ============================================================
-- 2. face_datas 테이블 (얼굴 랜드마크 데이터)
-- ============================================================

DROP TABLE IF EXISTS face_datas;

CREATE TABLE face_datas (
    face_data_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id VARCHAR(16) NOT NULL,
    s3_url VARCHAR(512) NOT NULL COMMENT '얼굴 좌표 txt 파일 S3 URL',
    face_image_url VARCHAR(512) COMMENT '원본 얼굴 이미지 S3 URL',
    landmark_count INT DEFAULT 68 COMMENT '얼굴 랜드마크 개수',
    confidence_score DECIMAL(5, 4) COMMENT '인식 신뢰도 (0.0000~1.0000)',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='얼굴 랜드마크 좌표 데이터';

-- ============================================================
-- 3. body_datas 테이블 (체형 데이터)
-- ============================================================

DROP TABLE IF EXISTS body_datas;

CREATE TABLE body_datas (
    body_data_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id VARCHAR(16) NOT NULL,
    height DECIMAL(5, 2) NOT NULL COMMENT '키 (cm)',
    shoulder_width DECIMAL(5, 2) COMMENT '어깨 너비 (cm)',
    torso_length DECIMAL(5, 2) COMMENT '상체 길이 (cm)',
    leg_length DECIMAL(5, 2) COMMENT '다리 길이 (cm)',
    arm_length DECIMAL(5, 2) COMMENT '팔 길이 (cm)',
    body_image_url VARCHAR(512) COMMENT '전신 사진 S3 URL',
    overlay_image_url VARCHAR(512) COMMENT '측정 오버레이 이미지 S3 URL',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='사용자 체형 데이터';

-- ============================================================
-- 4. user_profiles 테이블 (맞춤형 차량 세팅)
-- ============================================================

DROP TABLE IF EXISTS user_profiles;

CREATE TABLE user_profiles (
    profile_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id VARCHAR(16) NOT NULL,
    seat_position JSON COMMENT '시트 위치 {"forward": 50, "height": 30, "recline": 20}',
    mirror_position JSON COMMENT '미러 위치 {"left": {...}, "right": {...}, "rear": {...}}',
    steering_position JSON COMMENT '스티어링 위치 {"height": 40, "depth": 50}',
    climate_preference JSON COMMENT '선호 온도 {"temp": 22, "fan": 3, "mode": "auto"}',
    status ENUM('generating', 'completed', 'failed') DEFAULT 'generating',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='사용자 맞춤형 차량 세팅';

-- ============================================================
-- 5. cars 테이블 (차량 정보)
-- ============================================================

DROP TABLE IF EXISTS cars;

CREATE TABLE cars (
    car_id VARCHAR(16) NOT NULL PRIMARY KEY COMMENT '차량 고유 ID',
    plate_number VARCHAR(20) NOT NULL UNIQUE COMMENT '차량 번호판',
    model VARCHAR(100) NOT NULL COMMENT '차량 모델',
    manufacturer VARCHAR(50) COMMENT '제조사',
    year INT COMMENT '제조년도',
    color VARCHAR(30) COMMENT '색상',
    status ENUM('available', 'rented', 'maintenance', 'unavailable') DEFAULT 'available',
    location_lat DECIMAL(10, 8) COMMENT '현재 위치 위도',
    location_lng DECIMAL(11, 8) COMMENT '현재 위치 경도',
    location_address VARCHAR(255) COMMENT '현재 위치 주소',
    image_url VARCHAR(512) COMMENT '차량 이미지 URL',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    INDEX idx_status (status),
    INDEX idx_location (location_lat, location_lng),
    INDEX idx_plate (plate_number)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='차량 정보';

-- ============================================================
-- 6. bookings 테이블 (차량 예약/대여)
-- ============================================================

DROP TABLE IF EXISTS bookings;

CREATE TABLE bookings (
    booking_id VARCHAR(16) NOT NULL PRIMARY KEY COMMENT '예약 고유 ID',
    user_id VARCHAR(16) NOT NULL,
    car_id VARCHAR(16) NOT NULL,
    start_time TIMESTAMP NOT NULL COMMENT '대여 시작 시간',
    end_time TIMESTAMP NOT NULL COMMENT '대여 종료 시간',
    status ENUM('pending', 'approved', 'active', 'completed', 'cancelled') DEFAULT 'pending',
    approved_at TIMESTAMP NULL COMMENT '승인 시간',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (car_id) REFERENCES cars(car_id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_car_id (car_id),
    INDEX idx_status (status),
    INDEX idx_user_status (user_id, status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='차량 예약 및 대여';

-- ============================================================
-- 7. auth_sessions 테이블 (MFA 인증 세션)
-- ============================================================

DROP TABLE IF EXISTS auth_sessions;

CREATE TABLE auth_sessions (
    session_id VARCHAR(32) NOT NULL PRIMARY KEY COMMENT '세션 고유 ID',
    booking_id VARCHAR(16) NOT NULL,
    user_id VARCHAR(16) NOT NULL,
    car_id VARCHAR(16) NOT NULL,
    nfc_verified BOOLEAN DEFAULT FALSE,
    ble_verified BOOLEAN DEFAULT FALSE,
    face_verified BOOLEAN DEFAULT FALSE,
    status ENUM('pending', 'in_progress', 'completed', 'failed', 'expired') DEFAULT 'pending',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NOT NULL COMMENT '세션 만료 시간 (생성 + 10분)',

    FOREIGN KEY (booking_id) REFERENCES bookings(booking_id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (car_id) REFERENCES cars(car_id) ON DELETE CASCADE,
    INDEX idx_session_status (session_id, status),
    INDEX idx_booking (booking_id),
    INDEX idx_expires (expires_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='MFA 인증 세션';

-- ============================================================
-- 8. ble_sessions 테이블 (BLE 인증 세션)
-- ============================================================

DROP TABLE IF EXISTS ble_sessions;

CREATE TABLE ble_sessions (
    session_id VARCHAR(32) NOT NULL PRIMARY KEY,
    user_id VARCHAR(16) NOT NULL,
    car_id VARCHAR(16) NOT NULL,
    hashkey VARCHAR(16) NOT NULL COMMENT 'BLE 해시키',
    nonce VARCHAR(32) NOT NULL COMMENT 'Nonce',
    expires_at TIMESTAMP NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    KEY idx_user_car (user_id, car_id),
    KEY idx_hashkey (hashkey),
    KEY idx_expires_at (expires_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='BLE 인증 세션';

-- ============================================================
-- 9. vehicle_settings 테이블 (차량 설정 기록)
-- ============================================================

DROP TABLE IF EXISTS vehicle_settings;

CREATE TABLE vehicle_settings (
    setting_id INT AUTO_INCREMENT PRIMARY KEY,
    car_id VARCHAR(16) NOT NULL,
    user_id VARCHAR(16) NOT NULL,
    seat_position JSON,
    mirror_position JSON,
    steering_position JSON,
    climate_settings JSON,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (car_id) REFERENCES cars(car_id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_car_user (car_id, user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='차량 설정 적용 기록';

-- ============================================================
-- 완료
-- ============================================================

SELECT 'Database schema created successfully!' AS message;
