-- ============================================================
-- HYPERMOB Database Seed Data
-- ============================================================
-- 목적: 통합 데이터베이스 테스트용 더미 데이터
-- 실행 순서: unified-schema.sql 실행 후 이 파일 실행
-- ============================================================

USE hypermob;

-- ============================================================
-- 1. users 테이블 더미 데이터
-- ============================================================

INSERT INTO users (user_id, name, email, phone, address, face_id, nfc_uid, profile_image_url, created_at, updated_at) VALUES
('U000000000000001', '김민수', 'minsu.kim@example.com', '010-1234-5678', '서울특별시 강남구 테헤란로 123', 'F000000000000001', '04A1B2C3D4E5F6', 'https://hypermob-profiles.s3.amazonaws.com/users/U000000000000001/profile.jpg', NOW(), NOW()),
('U000000000000002', '이지은', 'jieun.lee@example.com', '010-2345-6789', '서울특별시 서초구 반포대로 456', 'F000000000000002', '04B2C3D4E5F6A1', 'https://hypermob-profiles.s3.amazonaws.com/users/U000000000000002/profile.jpg', NOW(), NOW()),
('U000000000000003', '박준호', 'junho.park@example.com', '010-3456-7890', '서울특별시 송파구 올림픽로 789', 'F000000000000003', '04C3D4E5F6A1B2', 'https://hypermob-profiles.s3.amazonaws.com/users/U000000000000003/profile.jpg', NOW(), NOW()),
('U000000000000004', '최수진', 'sujin.choi@example.com', '010-4567-8901', '경기도 성남시 분당구 판교역로 321', 'F000000000000004', '04D4E5F6A1B2C3', 'https://hypermob-profiles.s3.amazonaws.com/users/U000000000000004/profile.jpg', NOW(), NOW()),
('U000000000000005', '정현우', 'hyunwoo.jung@example.com', '010-5678-9012', '인천광역시 연수구 송도과학로 654', NULL, NULL, NULL, NOW(), NOW());

-- ============================================================
-- 2. face_datas 테이블 더미 데이터
-- ============================================================

INSERT INTO face_datas (user_id, s3_url, face_image_url, landmark_count, confidence_score, created_at) VALUES
('U000000000000001', 'https://hypermob-facedata.s3.amazonaws.com/users/U000000000000001/landmarks_001.txt', 'https://hypermob-facedata.s3.amazonaws.com/users/U000000000000001/face_001.jpg', 68, 0.9823, NOW()),
('U000000000000001', 'https://hypermob-facedata.s3.amazonaws.com/users/U000000000000001/landmarks_002.txt', 'https://hypermob-facedata.s3.amazonaws.com/users/U000000000000001/face_002.jpg', 68, 0.9756, NOW()),
('U000000000000002', 'https://hypermob-facedata.s3.amazonaws.com/users/U000000000000002/landmarks_001.txt', 'https://hypermob-facedata.s3.amazonaws.com/users/U000000000000002/face_001.jpg', 68, 0.9891, NOW()),
('U000000000000002', 'https://hypermob-facedata.s3.amazonaws.com/users/U000000000000002/landmarks_002.txt', 'https://hypermob-facedata.s3.amazonaws.com/users/U000000000000002/face_002.jpg', 68, 0.9834, NOW()),
('U000000000000003', 'https://hypermob-facedata.s3.amazonaws.com/users/U000000000000003/landmarks_001.txt', 'https://hypermob-facedata.s3.amazonaws.com/users/U000000000000003/face_001.jpg', 68, 0.9712, NOW()),
('U000000000000004', 'https://hypermob-facedata.s3.amazonaws.com/users/U000000000000004/landmarks_001.txt', 'https://hypermob-facedata.s3.amazonaws.com/users/U000000000000004/face_001.jpg', 68, 0.9645, NOW());

-- ============================================================
-- 3. body_datas 테이블 더미 데이터
-- ============================================================

INSERT INTO body_datas (user_id, height, shoulder_width, torso_length, leg_length, arm_length, body_image_url, overlay_image_url, created_at) VALUES
('U000000000000001', 175.50, 45.20, 72.30, 88.50, 58.70, 'https://hypermob-bodydata.s3.amazonaws.com/users/U000000000000001/body_001.jpg', 'https://hypermob-bodydata.s3.amazonaws.com/users/U000000000000001/overlay_001.jpg', NOW()),
('U000000000000002', 162.30, 38.50, 65.80, 82.40, 54.20, 'https://hypermob-bodydata.s3.amazonaws.com/users/U000000000000002/body_001.jpg', 'https://hypermob-bodydata.s3.amazonaws.com/users/U000000000000002/overlay_001.jpg', NOW()),
('U000000000000003', 182.00, 48.70, 76.50, 92.30, 61.80, 'https://hypermob-bodydata.s3.amazonaws.com/users/U000000000000003/body_001.jpg', 'https://hypermob-bodydata.s3.amazonaws.com/users/U000000000000003/overlay_001.jpg', NOW()),
('U000000000000004', 168.70, 41.30, 68.90, 85.20, 56.50, 'https://hypermob-bodydata.s3.amazonaws.com/users/U000000000000004/body_001.jpg', 'https://hypermob-bodydata.s3.amazonaws.com/users/U000000000000004/overlay_001.jpg', NOW());

-- ============================================================
-- 4. user_profiles 테이블 더미 데이터
-- ============================================================

INSERT INTO user_profiles (user_id, seat_position, mirror_position, steering_position, climate_preference, status, created_at, updated_at) VALUES
('U000000000000001',
 '{"forward": 50, "height": 35, "recline": 22}',
 '{"left": {"horizontal": 45, "vertical": 50}, "right": {"horizontal": 55, "vertical": 50}, "rear": {"tilt": 30}}',
 '{"height": 42, "depth": 48}',
 '{"temp": 22, "fan": 3, "mode": "auto"}',
 'completed', NOW(), NOW()),

('U000000000000002',
 '{"forward": 45, "height": 30, "recline": 20}',
 '{"left": {"horizontal": 42, "vertical": 48}, "right": {"horizontal": 58, "vertical": 52}, "rear": {"tilt": 28}}',
 '{"height": 38, "depth": 45}',
 '{"temp": 23, "fan": 2, "mode": "auto"}',
 'completed', NOW(), NOW()),

('U000000000000003',
 '{"forward": 55, "height": 40, "recline": 25}',
 '{"left": {"horizontal": 48, "vertical": 52}, "right": {"horizontal": 52, "vertical": 48}, "rear": {"tilt": 32}}',
 '{"height": 45, "depth": 52}',
 '{"temp": 21, "fan": 4, "mode": "cool"}',
 'completed', NOW(), NOW()),

('U000000000000004',
 '{"forward": 48, "height": 32, "recline": 21}',
 '{"left": {"horizontal": 44, "vertical": 49}, "right": {"horizontal": 56, "vertical": 51}, "rear": {"tilt": 29}}',
 '{"height": 40, "depth": 47}',
 '{"temp": 24, "fan": 3, "mode": "auto"}',
 'completed', NOW(), NOW());

-- ============================================================
-- 5. cars 테이블 더미 데이터
-- ============================================================

INSERT INTO cars (car_id, plate_number, model, manufacturer, year, color, status, location_lat, location_lng, location_address, image_url, created_at, updated_at) VALUES
('C000000000000001', '12가3456', 'GV80', 'Genesis', 2023, 'White', 'rented', 37.5665, 126.9780, '서울특별시 중구 을지로 100', 'https://hypermob-cars.s3.amazonaws.com/cars/C000000000000001.jpg', NOW(), NOW()),
('C000000000000002', '34나5678', 'EV6', 'Kia', 2024, 'Black', 'available', 37.4979, 127.0276, '서울특별시 강남구 테헤란로 152', 'https://hypermob-cars.s3.amazonaws.com/cars/C000000000000002.jpg', NOW(), NOW()),
('C000000000000003', '56다7890', 'IONIQ 5', 'Hyundai', 2023, 'Blue', 'available', 37.5172, 127.0473, '서울특별시 강남구 삼성로 567', 'https://hypermob-cars.s3.amazonaws.com/cars/C000000000000003.jpg', NOW(), NOW()),
('C000000000000004', '78라9012', 'G90', 'Genesis', 2024, 'Silver', 'rented', 37.5048, 127.0495, '서울특별시 강남구 영동대로 513', 'https://hypermob-cars.s3.amazonaws.com/cars/C000000000000004.jpg', NOW(), NOW()),
('C000000000000005', '90마1234', 'Carnival', 'Kia', 2023, 'Gray', 'available', 37.4833, 127.0322, '서울특별시 서초구 강남대로 465', 'https://hypermob-cars.s3.amazonaws.com/cars/C000000000000005.jpg', NOW(), NOW()),
('C000000000000006', '23바4567', 'Palisade', 'Hyundai', 2024, 'Green', 'maintenance', 37.5112, 127.0590, '서울특별시 송파구 올림픽로 300', 'https://hypermob-cars.s3.amazonaws.com/cars/C000000000000006.jpg', NOW(), NOW()),
('C000000000000007', '45사6789', 'GV70', 'Genesis', 2023, 'Red', 'available', 37.5407, 127.0696, '서울특별시 광진구 능동로 120', 'https://hypermob-cars.s3.amazonaws.com/cars/C000000000000007.jpg', NOW(), NOW()),
('C000000000000008', '67아8901', 'EV9', 'Kia', 2024, 'White', 'available', 37.4563, 126.7052, '인천광역시 연수구 송도과학로 123', 'https://hypermob-cars.s3.amazonaws.com/cars/C000000000000008.jpg', NOW(), NOW());

-- ============================================================
-- 6. bookings 테이블 더미 데이터
-- ============================================================

INSERT INTO bookings (booking_id, user_id, car_id, start_time, end_time, status, approved_at, created_at, updated_at) VALUES
('B000000000000001', 'U000000000000001', 'C000000000000001', '2025-10-28 09:00:00', '2025-10-28 18:00:00', 'active', '2025-10-27 15:30:00', '2025-10-27 14:00:00', NOW()),
('B000000000000002', 'U000000000000002', 'C000000000000004', '2025-10-28 10:00:00', '2025-10-28 17:00:00', 'active', '2025-10-27 16:45:00', '2025-10-27 15:20:00', NOW()),
('B000000000000003', 'U000000000000003', 'C000000000000002', '2025-10-29 08:00:00', '2025-10-29 20:00:00', 'approved', '2025-10-28 10:00:00', '2025-10-28 09:15:00', NOW()),
('B000000000000004', 'U000000000000004', 'C000000000000003', '2025-10-29 09:00:00', '2025-10-29 19:00:00', 'pending', NULL, '2025-10-28 08:30:00', NOW()),
('B000000000000005', 'U000000000000001', 'C000000000000005', '2025-10-30 07:00:00', '2025-10-30 22:00:00', 'pending', NULL, '2025-10-28 07:45:00', NOW()),
('B000000000000006', 'U000000000000002', 'C000000000000007', '2025-10-25 10:00:00', '2025-10-25 16:00:00', 'completed', '2025-10-24 14:20:00', '2025-10-24 13:00:00', '2025-10-25 16:05:00'),
('B000000000000007', 'U000000000000003', 'C000000000000008', '2025-10-26 11:00:00', '2025-10-26 15:00:00', 'completed', '2025-10-25 17:30:00', '2025-10-25 16:45:00', '2025-10-26 15:10:00');

-- ============================================================
-- 7. auth_sessions 테이블 더미 데이터
-- ============================================================

INSERT INTO auth_sessions (session_id, booking_id, user_id, car_id, nfc_verified, ble_verified, face_verified, status, created_at, updated_at, expires_at) VALUES
('a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6', 'B000000000000001', 'U000000000000001', 'C000000000000001', TRUE, TRUE, TRUE, 'completed', '2025-10-28 08:55:00', '2025-10-28 09:02:00', '2025-10-28 09:05:00'),
('b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7', 'B000000000000002', 'U000000000000002', 'C000000000000004', TRUE, TRUE, TRUE, 'completed', '2025-10-28 09:55:00', '2025-10-28 10:03:00', '2025-10-28 10:05:00'),
('c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8', 'B000000000000003', 'U000000000000003', 'C000000000000002', TRUE, FALSE, FALSE, 'in_progress', '2025-10-28 10:30:00', '2025-10-28 10:32:00', '2025-10-28 10:40:00');

-- ============================================================
-- 8. ble_sessions 테이블 더미 데이터
-- ============================================================

INSERT INTO ble_sessions (session_id, user_id, car_id, hashkey, nonce, expires_at, created_at) VALUES
('d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9', 'U000000000000001', 'C000000000000001', 'H1A2B3C4D5E6F7G8', 'n1o2n3c4e5a6b7c8d9e0f1a2b3c4d5e6', '2025-10-28 09:05:00', '2025-10-28 08:55:00'),
('e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0', 'U000000000000002', 'C000000000000004', 'H2B3C4D5E6F7G8H9', 'n2o3n4c5e6a7b8c9d0e1f2a3b4c5d6e7', '2025-10-28 10:05:00', '2025-10-28 09:55:00'),
('f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1', 'U000000000000003', 'C000000000000002', 'H3C4D5E6F7G8H9I0', 'n3o4n5c6e7a8b9c0d1e2f3a4b5c6d7e8', '2025-10-28 10:40:00', '2025-10-28 10:30:00');

-- ============================================================
-- 9. vehicle_settings 테이블 더미 데이터
-- ============================================================

INSERT INTO vehicle_settings (car_id, user_id, seat_position, mirror_position, steering_position, climate_settings, created_at) VALUES
('C000000000000001', 'U000000000000001',
 '{"forward": 50, "height": 35, "recline": 22}',
 '{"left": {"horizontal": 45, "vertical": 50}, "right": {"horizontal": 55, "vertical": 50}, "rear": {"tilt": 30}}',
 '{"height": 42, "depth": 48}',
 '{"temp": 22, "fan": 3, "mode": "auto"}',
 '2025-10-28 09:00:00'),

('C000000000000004', 'U000000000000002',
 '{"forward": 45, "height": 30, "recline": 20}',
 '{"left": {"horizontal": 42, "vertical": 48}, "right": {"horizontal": 58, "vertical": 52}, "rear": {"tilt": 28}}',
 '{"height": 38, "depth": 45}',
 '{"temp": 23, "fan": 2, "mode": "auto"}',
 '2025-10-28 10:00:00'),

('C000000000000007', 'U000000000000002',
 '{"forward": 45, "height": 30, "recline": 20}',
 '{"left": {"horizontal": 42, "vertical": 48}, "right": {"horizontal": 58, "vertical": 52}, "rear": {"tilt": 28}}',
 '{"height": 38, "depth": 45}',
 '{"temp": 23, "fan": 2, "mode": "auto"}',
 '2025-10-26 11:00:00'),

('C000000000000008', 'U000000000000003',
 '{"forward": 55, "height": 40, "recline": 25}',
 '{"left": {"horizontal": 48, "vertical": 52}, "right": {"horizontal": 52, "vertical": 48}, "rear": {"tilt": 32}}',
 '{"height": 45, "depth": 52}',
 '{"temp": 21, "fan": 4, "mode": "cool"}',
 '2025-10-26 11:00:00');

-- ============================================================
-- 데이터 확인
-- ============================================================

SELECT 'Seed data inserted successfully!' AS message;

SELECT
  (SELECT COUNT(*) FROM users) AS users_count,
  (SELECT COUNT(*) FROM face_datas) AS face_datas_count,
  (SELECT COUNT(*) FROM body_datas) AS body_datas_count,
  (SELECT COUNT(*) FROM user_profiles) AS user_profiles_count,
  (SELECT COUNT(*) FROM cars) AS cars_count,
  (SELECT COUNT(*) FROM bookings) AS bookings_count,
  (SELECT COUNT(*) FROM auth_sessions) AS auth_sessions_count,
  (SELECT COUNT(*) FROM ble_sessions) AS ble_sessions_count,
  (SELECT COUNT(*) FROM vehicle_settings) AS vehicle_settings_count;
