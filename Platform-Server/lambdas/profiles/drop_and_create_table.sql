-- Drop existing profiles table
DROP TABLE IF EXISTS profiles;

-- Create optimized profiles table for body measurements and vehicle settings
CREATE TABLE profiles (
    id INT AUTO_INCREMENT PRIMARY KEY,
    profile_id VARCHAR(50) UNIQUE NOT NULL,
    user_id VARCHAR(50) NOT NULL,

    -- Body measurements (from body scan)
    sitting_height DECIMAL(5,2) DEFAULT NULL,
    shoulder_width DECIMAL(5,2) DEFAULT NULL,
    arm_length DECIMAL(5,2) DEFAULT NULL,
    head_height DECIMAL(5,2) DEFAULT NULL,
    eye_height DECIMAL(5,2) DEFAULT NULL,
    leg_length DECIMAL(5,2) DEFAULT NULL,
    torso_length DECIMAL(5,2) DEFAULT NULL,
    weight DECIMAL(5,2) DEFAULT NULL,
    height DECIMAL(5,2) DEFAULT NULL,

    -- Vehicle settings (from /recommend API)
    seat_position INT DEFAULT NULL,
    seat_angle INT DEFAULT NULL,
    seat_front_height INT DEFAULT NULL,
    seat_rear_height INT DEFAULT NULL,
    mirror_left_yaw INT DEFAULT NULL,
    mirror_left_pitch INT DEFAULT NULL,
    mirror_right_yaw INT DEFAULT NULL,
    mirror_right_pitch INT DEFAULT NULL,
    mirror_room_yaw INT DEFAULT NULL,
    mirror_room_pitch INT DEFAULT NULL,
    wheel_position INT DEFAULT NULL,
    wheel_angle INT DEFAULT NULL,

    -- Timestamps
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    -- Indexes
    INDEX idx_user_id (user_id),
    INDEX idx_profile_id (profile_id),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
