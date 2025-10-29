package com.hypermob.mydrive3dx.domain.repository

import com.hypermob.mydrive3dx.domain.model.*
import kotlinx.coroutines.flow.Flow

/**
 * Body Scan Repository Interface
 *
 * 3D 바디스캔 데이터 및 프로필 관리
 */
interface BodyScanRepository {

    /**
     * 바디스캔 데이터를 서버로 전송하여 프로필 생성
     */
    suspend fun createProfile(
        userId: String,
        bodyScanData: BodyScan,
        vehicleId: String? = null
    ): Flow<Result<BodyProfile>>

    /**
     * 사용자의 바디 프로필 조회
     */
    suspend fun getProfile(
        userId: String,
        vehicleId: String? = null
    ): Flow<Result<BodyProfile>>

    /**
     * 모든 프로필 목록 조회
     */
    suspend fun getAllProfiles(userId: String): Flow<Result<List<BodyProfile>>>

    /**
     * 프로필 활성화/비활성화
     */
    suspend fun setActiveProfile(profileId: String): Flow<Result<Unit>>

    /**
     * 프로필 삭제
     */
    suspend fun deleteProfile(profileId: String): Flow<Result<Unit>>

    /**
     * 차량에 프로필 기반 세팅 적용
     */
    suspend fun applyVehicleSettings(
        profileId: String,
        vehicleId: String
    ): Flow<Result<VehicleSettingResult>>

    /**
     * 차량 세팅 수동 업데이트 (사용자 피드백 반영)
     */
    suspend fun updateVehicleSettings(
        profileId: String,
        settings: VehicleSettings
    ): Flow<Result<BodyProfile>>

    /**
     * 전신 사진 업로드
     */
    suspend fun uploadBodyImage(
        userId: String,
        imageBytes: ByteArray,
        height: Float? = null
    ): Flow<Result<BodyImageUploadResult>>

    /**
     * 체형 데이터 생성 (시나리오1 - Step 9)
     */
    suspend fun generateMeasurement(
        userId: String,
        imageS3Path: String,
        heightCm: Int
    ): Flow<Result<MeasurementResult>>

    /**
     * 맞춤형 차량 프로필 생성 (시나리오1 - Step 10)
     * @deprecated Use generateRecommendationWithMeasurements instead
     */
    suspend fun generateRecommendation(
        userId: String,
        measurementId: String
    ): Flow<Result<RecommendationResult>>

    /**
     * 맞춤형 차량 프로필 생성 with measurements
     */
    suspend fun generateRecommendationWithMeasurements(
        userId: String,
        height: Double,
        upperArm: Double,
        forearm: Double,
        thigh: Double,
        calf: Double,
        torso: Double
    ): Flow<Result<RecommendationResult>>
}

/**
 * Body Image Upload Result
 */
data class BodyImageUploadResult(
    val success: Boolean,
    val s3Path: String? = null,
    val bodyDataId: String? = null,
    val message: String? = null
)

/**
 * Measurement Result
 */
data class MeasurementResult(
    val success: Boolean,
    val measurementId: String? = null,
    val shoulderWidth: Double? = null,
    val armLength: Double? = null,
    val torsoHeight: Double? = null,
    val legLength: Double? = null,
    val bodyType: String? = null,
    val message: String? = null,
    val upperArmCm: Double? = null,
    val forearmCm: Double? = null,
    val thighCm: Double? = null,
    val calfCm: Double? = null,
    val torsoCm: Double? = null
)

/**
 * Recommendation Result
 */
data class RecommendationResult(
    val success: Boolean,
    val profileId: String? = null,
    val seatPosition: Int? = null,
    val seatHeight: Int? = null,
    val steeringWheelPosition: Int? = null,
    val mirrorAngle: Int? = null,
    val message: String? = null
)
