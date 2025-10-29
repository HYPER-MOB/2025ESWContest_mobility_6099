package com.hypermob.mydrive3dx.domain.model

import kotlinx.serialization.Serializable

/**
 * User Profile with Body Measurements and Vehicle Settings
 * 사용자 프로필 (신체 측정 및 차량 설정 포함)
 */
@Serializable
data class UserProfile(
    val user_id: String,
    val body_measurements: BodyMeasurements,
    val vehicle_settings: UserVehicleSettings,
    val has_profile: Boolean = true,
    val created_at: String? = null,
    val updated_at: String? = null
)

/**
 * Body Measurements
 * 신체 측정값 (6가지)
 */
@Serializable
data class BodyMeasurements(
    val height: Double,        // cm - 신장
    val upper_arm: Double,     // cm - 상완 길이
    val forearm: Double,       // cm - 전완 길이
    val thigh: Double,         // cm - 대퇴 길이
    val calf: Double,          // cm - 종아리 길이
    val torso: Double          // cm - 상체 길이
)

/**
 * User Vehicle Settings (API 응답용)
 * 차량 설정값 (12가지)
 */
@Serializable
data class UserVehicleSettings(
    val seat_position: Double,         // 0-100 - 시트 위치
    val seat_angle: Double,            // 0-45 degrees - 시트 각도
    val seat_front_height: Double,     // 0-100 - 시트 앞부분 높이
    val seat_rear_height: Double,      // 0-100 - 시트 뒷부분 높이
    val mirror_left_yaw: Double,       // 0-360 degrees - 좌측 사이드미러 좌우
    val mirror_left_pitch: Double,     // -30 to 30 degrees - 좌측 사이드미러 상하
    val mirror_right_yaw: Double,      // 0-360 degrees - 우측 사이드미러 좌우
    val mirror_right_pitch: Double,    // -30 to 30 degrees - 우측 사이드미러 상하
    val mirror_room_yaw: Double,       // 0-360 degrees - 룸미러 좌우
    val mirror_room_pitch: Double,     // -30 to 30 degrees - 룸미러 상하
    val wheel_position: Double,        // 0-100 - 핸들 위치
    val wheel_angle: Double            // 0-45 degrees - 핸들 각도
)
