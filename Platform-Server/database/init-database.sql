-- =====================================================
-- HYPERMOB Unified Platform - Database Initialization
-- Version: 2.0
-- Description: Complete database schema with all tables
-- =====================================================

-- Create database if not exists
CREATE DATABASE IF NOT EXISTS hypermob
CHARACTER SET utf8mb4
COLLATE utf8mb4_unicode_ci;

USE hypermob;

-- =====================================================
-- 1. Users Table
-- =====================================================
DROP TABLE IF EXISTS users;
CREATE TABLE users (
    user_id VARCHAR(20) PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(255) NOT NULL,
    phone VARCHAR(20) NOT NULL,
    face_id VARCHAR(100) UNIQUE,
    profile_image_url TEXT,
    body_measurements JSON,
    vehicle_settings JSON,
    nfc_uid VARCHAR(20) UNIQUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_face_id (face_id),
    INDEX idx_nfc_uid (nfc_uid),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =====================================================
-- 2. Vehicles Table
-- =====================================================
DROP TABLE IF EXISTS vehicles;
CREATE TABLE vehicles (
    vehicle_id VARCHAR(20) PRIMARY KEY,
    user_id VARCHAR(20),
    vehicle_name VARCHAR(100) NOT NULL,
    vin VARCHAR(17) UNIQUE,
    model VARCHAR(50),
    year INT,
    color VARCHAR(30),
    license_plate VARCHAR(20),
    current_location JSON,
    battery_level INT DEFAULT 100,
    is_locked BOOLEAN DEFAULT true,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE SET NULL,
    INDEX idx_user_id (user_id),
    INDEX idx_vin (vin)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =====================================================
-- 3. Bookings Table
-- =====================================================
DROP TABLE IF EXISTS bookings;
CREATE TABLE bookings (
    booking_id VARCHAR(20) PRIMARY KEY,
    user_id VARCHAR(20),
    vehicle_id VARCHAR(20),
    start_time DATETIME NOT NULL,
    end_time DATETIME NOT NULL,
    start_location JSON,
    end_location JSON,
    total_distance DECIMAL(10,2),
    total_cost DECIMAL(10,2),
    status ENUM('pending', 'confirmed', 'active', 'completed', 'cancelled') DEFAULT 'pending',
    payment_status ENUM('pending', 'paid', 'refunded') DEFAULT 'pending',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (vehicle_id) REFERENCES vehicles(vehicle_id) ON DELETE CASCADE,
    INDEX idx_user_bookings (user_id, status),
    INDEX idx_vehicle_bookings (vehicle_id, start_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =====================================================
-- 4. Sessions Table
-- =====================================================
DROP TABLE IF EXISTS sessions;
CREATE TABLE sessions (
    session_id VARCHAR(36) PRIMARY KEY,
    user_id VARCHAR(20),
    token VARCHAR(255) UNIQUE NOT NULL,
    device_info JSON,
    ip_address VARCHAR(45),
    is_active BOOLEAN DEFAULT true,
    expires_at DATETIME NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_activity TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_token (token),
    INDEX idx_user_sessions (user_id, is_active),
    INDEX idx_expires_at (expires_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =====================================================
-- 5. AI Analysis Results Table
-- =====================================================
DROP TABLE IF EXISTS ai_analysis_results;
CREATE TABLE ai_analysis_results (
    analysis_id VARCHAR(36) PRIMARY KEY DEFAULT (UUID()),
    user_id VARCHAR(20),
    analysis_type ENUM('facedata', 'measure', 'recommend') NOT NULL,
    input_data JSON,
    result_data JSON,
    s3_result_url TEXT,
    processing_time_ms INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    INDEX idx_user_analysis (user_id, analysis_type, created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =====================================================
-- 6. Vehicle Settings History Table
-- =====================================================
DROP TABLE IF EXISTS vehicle_settings_history;
CREATE TABLE vehicle_settings_history (
    history_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id VARCHAR(20),
    vehicle_id VARCHAR(20),
    seat_position INT,
    seat_angle INT,
    seat_front_height INT,
    seat_rear_height INT,
    mirror_left_yaw INT,
    mirror_left_pitch INT,
    mirror_right_yaw INT,
    mirror_right_pitch INT,
    mirror_room_yaw INT,
    mirror_room_pitch INT,
    wheel_position INT,
    wheel_angle INT,
    applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (vehicle_id) REFERENCES vehicles(vehicle_id) ON DELETE CASCADE,
    INDEX idx_user_settings (user_id, applied_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =====================================================
-- 7. Audit Log Table
-- =====================================================
DROP TABLE IF EXISTS audit_logs;
CREATE TABLE audit_logs (
    log_id BIGINT AUTO_INCREMENT PRIMARY KEY,
    user_id VARCHAR(20),
    action VARCHAR(50) NOT NULL,
    entity_type VARCHAR(50),
    entity_id VARCHAR(36),
    old_values JSON,
    new_values JSON,
    ip_address VARCHAR(45),
    user_agent TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_user_logs (user_id, created_at),
    INDEX idx_entity_logs (entity_type, entity_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =====================================================
-- 8. Sample Data for Testing
-- =====================================================
-- Insert sample vehicle
INSERT INTO vehicles (vehicle_id, vehicle_name, vin, model, year, color, license_plate, current_location)
VALUES
    ('VEH001', 'Tesla Model 3', '5YJ3E1EA1JF000001', 'Model 3', 2023, 'Pearl White', '서울12가3456',
     JSON_OBJECT('lat', 37.5665, 'lng', 126.9780, 'address', '서울특별시 중구'));

-- Create stored procedures for common operations
DELIMITER //

-- Procedure to clean up expired sessions
CREATE PROCEDURE cleanup_expired_sessions()
BEGIN
    UPDATE sessions
    SET is_active = false
    WHERE expires_at < NOW() AND is_active = true;

    DELETE FROM sessions
    WHERE expires_at < DATE_SUB(NOW(), INTERVAL 30 DAY);
END//

-- Procedure to get user with latest settings
CREATE PROCEDURE get_user_with_settings(IN p_user_id VARCHAR(20))
BEGIN
    SELECT
        u.*,
        vsh.seat_position,
        vsh.seat_angle,
        vsh.wheel_position,
        vsh.wheel_angle
    FROM users u
    LEFT JOIN (
        SELECT * FROM vehicle_settings_history
        WHERE user_id = p_user_id
        ORDER BY applied_at DESC
        LIMIT 1
    ) vsh ON u.user_id = vsh.user_id
    WHERE u.user_id = p_user_id;
END//

DELIMITER ;

-- Grant permissions (adjust user as needed)
-- GRANT ALL PRIVILEGES ON hypermob.* TO 'admin'@'%' IDENTIFIED BY 'admin123';
-- FLUSH PRIVILEGES;

-- =====================================================
-- Verification Queries
-- =====================================================
SELECT 'Database initialization complete!' as status;
SHOW TABLES;