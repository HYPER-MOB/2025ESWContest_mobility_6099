package com.hypermob.mydrive3dx.data.remote.api

import com.hypermob.mydrive3dx.data.remote.dto.*
import retrofit2.http.*

/**
 * Body Scan API
 *
 * 3D 바디스캔 및 프로필 관련 API
 */
interface BodyScanApi {

    /**
     * 전신 사진 업로드 (서버 Lambda 호환)
     * POST /body/upload
     */
    @Multipart
    @POST("body/upload")
    suspend fun uploadBodyImage(
        @Part image: okhttp3.MultipartBody.Part,
        @Part("user_id") userId: okhttp3.RequestBody,
        @Part("height") height: okhttp3.RequestBody? = null
    ): BodyUploadResponse

    /**
     * 체형 데이터 생성
     * POST /measure
     */
    @POST("measure")
    suspend fun generateMeasurement(
        @Body request: MeasureRequest
    ): MeasureResponse

    /**
     * 맞춤형 차량 프로필 생성
     * POST /recommend
     */
    @POST("recommend")
    suspend fun generateRecommendation(
        @Body request: RecommendRequest
    ): RecommendResponse

    /**
     * 사용자 프로필 저장 (Body + Vehicle Settings)
     * POST /profiles/{user_id}
     */
    @POST("profiles/{user_id}")
    suspend fun saveProfile(
        @Path("user_id") userId: String,
        @Body request: ProfileSaveRequest
    ): ProfileSaveResponse

    /**
     * 사용자 프로필 조회 (Body + Vehicle Settings)
     * GET /profiles/{user_id}
     */
    @GET("profiles/{user_id}")
    suspend fun getProfileByUserId(
        @Path("user_id") userId: String
    ): ProfileSaveResponse

    /**
     * 바디스캔 데이터 업로드 및 프로필 생성
     */
    @POST("profiles/body-scan")
    suspend fun createProfile(
        @Body bodyScanData: BodyScanDataDto
    ): ApiResponse<BodyProfileDto>

    /**
     * 사용자 프로필 조회
     */
    @GET("profiles/{userId}")
    suspend fun getProfile(
        @Path("userId") userId: String,
        @Query("vehicleId") vehicleId: String? = null
    ): ApiResponse<BodyProfileDto>

    /**
     * 모든 프로필 목록 조회
     */
    @GET("profiles/user/{userId}/all")
    suspend fun getAllProfiles(
        @Path("userId") userId: String
    ): ApiResponse<List<BodyProfileDto>>

    /**
     * 프로필 활성화
     */
    @POST("profiles/{profileId}/activate")
    suspend fun setActiveProfile(
        @Path("profileId") profileId: String
    ): ApiResponse<Unit>

    /**
     * 프로필 삭제
     */
    @DELETE("profiles/{profileId}")
    suspend fun deleteProfile(
        @Path("profileId") profileId: String
    ): ApiResponse<Unit>

    /**
     * 차량에 프로필 세팅 적용
     */
    @POST("profiles/{profileId}/apply")
    suspend fun applyVehicleSettings(
        @Path("profileId") profileId: String,
        @Query("vehicleId") vehicleId: String
    ): ApiResponse<VehicleSettingResultDto>

    /**
     * 차량 세팅 수동 업데이트
     */
    @PUT("profiles/{profileId}/settings")
    suspend fun updateVehicleSettings(
        @Path("profileId") profileId: String,
        @Body settings: VehicleSettingsDto
    ): ApiResponse<BodyProfileDto>
}

/**
 * Body Upload Response DTO
 */
@kotlinx.serialization.Serializable
data class BodyUploadResponse(
    val body_image_url: String? = null,
    val message: String? = null
)

/**
 * Measure Request DTO
 */
@kotlinx.serialization.Serializable
data class MeasureRequest(
    val user_id: String,
    val image_url: String,
    val height_cm: Double
)

/**
 * Measure Response DTO
 */
@kotlinx.serialization.Serializable
data class MeasureResponse(
    val status: String,
    val data: MeasurementData? = null
)

@kotlinx.serialization.Serializable
data class MeasurementData(
    val upper_arm: Double? = null,
    val forearm: Double? = null,
    val thigh: Double? = null,
    val calf: Double? = null,
    val torso: Double? = null,
    val height: Double? = null,
    val scale_cm_per_px: Double? = null,
    val output_image_url: String? = null,
    val user_id: String? = null
)

/**
 * Recommend Request DTO
 */
@kotlinx.serialization.Serializable
data class RecommendRequest(
    val user_id: String,
    val height: Double,
    val upper_arm: Double,
    val forearm: Double,
    val thigh: Double,
    val calf: Double,
    val torso: Double
)

/**
 * Recommend Response DTO
 */
@kotlinx.serialization.Serializable
data class RecommendResponse(
    val status: String,
    val data: RecommendationData? = null
)

@kotlinx.serialization.Serializable
data class RecommendationData(
    val seat_position: Double? = null,
    val seat_angle: Double? = null,
    val seat_front_height: Double? = null,
    val seat_rear_height: Double? = null,
    val mirror_left_yaw: Double? = null,
    val mirror_left_pitch: Double? = null,
    val mirror_right_yaw: Double? = null,
    val mirror_right_pitch: Double? = null,
    val mirror_room_yaw: Double? = null,
    val mirror_room_pitch: Double? = null,
    val wheel_position: Double? = null,
    val wheel_angle: Double? = null
)

/**
 * Profile Save Request DTO
 */
@kotlinx.serialization.Serializable
data class ProfileSaveRequest(
    val bodyMeasurements: ProfileBodyMeasurements? = null,
    val vehicleSettings: ProfileVehicleSettings? = null
)

@kotlinx.serialization.Serializable
data class ProfileBodyMeasurements(
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

@kotlinx.serialization.Serializable
data class ProfileVehicleSettings(
    val seat_position: Int? = null,
    val seat_angle: Int? = null,
    val seat_front_height: Int? = null,
    val seat_rear_height: Int? = null,
    val mirror_left_yaw: Int? = null,
    val mirror_left_pitch: Int? = null,
    val mirror_right_yaw: Int? = null,
    val mirror_right_pitch: Int? = null,
    val mirror_room_yaw: Int? = null,
    val mirror_room_pitch: Int? = null,
    val wheel_position: Int? = null,
    val wheel_angle: Int? = null
)

/**
 * Profile Save Response DTO
 */
@kotlinx.serialization.Serializable
data class ProfileSaveResponse(
    val status: String,
    val data: ProfileSaveData? = null
)

@kotlinx.serialization.Serializable
data class ProfileSaveData(
    val profile_id: String? = null,
    val user_id: String? = null,
    val body_measurements: ProfileBodyMeasurements? = null,
    val vehicle_settings: ProfileVehicleSettings? = null,
    val created_at: String? = null,
    val updated_at: String? = null
)
