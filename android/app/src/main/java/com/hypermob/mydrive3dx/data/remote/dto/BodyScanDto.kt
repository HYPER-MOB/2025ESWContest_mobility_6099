package com.hypermob.mydrive3dx.data.remote.dto

import kotlinx.serialization.Serializable

/**
 * Body Scan DTO
 */
@Serializable
data class BodyScanDto(
    val scanId: String? = null,
    val userId: String? = null,
    val timestamp: String? = null,
    val sittingHeight: Double? = null,
    val shoulderWidth: Double? = null,
    val armLength: Double? = null,
    val headHeight: Double? = null,
    val eyeHeight: Double? = null,
    val legLength: Double? = null,
    val torsoLength: Double? = null,
    val weight: Double? = null,
    val height: Double? = null
)

/**
 * Body Profile DTO
 */
@Serializable
data class BodyProfileDto(
    val profileId: String? = null,
    val userId: String? = null,
    val userName: String? = null,
    val bodyScan: BodyScanDto? = null,
    val vehicleSettings: VehicleSettingsDto? = null,
    val createdAt: String? = null,
    val updatedAt: String? = null,
    val isActive: Boolean = true
)

/**
 * Vehicle Settings DTO
 */
@Serializable
data class VehicleSettingsDto(
    val seatFrontBack: Double? = null,
    val seatUpDown: Double? = null,
    val seatBackAngle: Double? = null,
    val seatHeadrestHeight: Double? = null,
    val leftMirrorAngle: Double? = null,
    val rightMirrorAngle: Double? = null,
    val steeringWheelHeight: Double? = null,
    val steeringWheelDepth: Double? = null,
    val vehicleModel: String? = null,
    val calibrationVersion: String = "1.0"
)

/**
 * Body Scan Request DTO
 */
@Serializable
data class BodyScanRequestDto(
    val userId: String? = null,
    val vehicleId: String? = null
)

/**
 * Body Scan Data Upload DTO
 *
 * 클라이언트에서 측정한 바디스캔 데이터를 서버로 전송
 */
@Serializable
data class BodyScanDataDto(
    val userId: String? = null,
    val vehicleId: String? = null,
    val sittingHeight: Double? = null,
    val shoulderWidth: Double? = null,
    val armLength: Double? = null,
    val headHeight: Double? = null,
    val eyeHeight: Double? = null,
    val legLength: Double? = null,
    val torsoLength: Double? = null,
    val weight: Double? = null,
    val height: Double? = null,
    val rawLandmarks: String? = null  // Mediapipe Pose 랜드마크 JSON (선택적)
)

/**
 * Vehicle Setting Application Result DTO
 */
@Serializable
data class VehicleSettingResultDto(
    val success: Boolean? = null,
    val profileId: String? = null,
    val appliedSettings: VehicleSettingsDto? = null,
    val estimatedTime: Int? = null,
    val message: String? = null
)

/**
 * Get Profile Request DTO
 */
@Serializable
data class GetProfileRequestDto(
    val userId: String? = null,
    val vehicleId: String? = null
)
