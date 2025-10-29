-- profiles 테이블 생성
CREATE TABLE IF NOT EXISTS profiles (
    id INT AUTO_INCREMENT PRIMARY KEY,
    profile_id VARCHAR(50) UNIQUE NOT NULL,
    user_id VARCHAR(50) NOT NULL,
    vehicle_id VARCHAR(50),

    -- 체형 7지표 + 추가 정보
    sitting_height DECIMAL(5,2) DEFAULT 0,
    shoulder_width DECIMAL(5,2) DEFAULT 0,
    arm_length DECIMAL(5,2) DEFAULT 0,
    head_height DECIMAL(5,2) DEFAULT 0,
    eye_height DECIMAL(5,2) DEFAULT 0,
    leg_length DECIMAL(5,2) DEFAULT 0,
    torso_length DECIMAL(5,2) DEFAULT 0,
    weight DECIMAL(5,2) DEFAULT 0,
    height DECIMAL(5,2) DEFAULT 0,

    -- 메타데이터
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    -- 인덱스
    INDEX idx_user_id (user_id),
    INDEX idx_profile_id (profile_id),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
