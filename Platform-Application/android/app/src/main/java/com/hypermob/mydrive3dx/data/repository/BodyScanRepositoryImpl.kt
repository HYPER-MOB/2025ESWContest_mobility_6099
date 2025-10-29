package com.hypermob.mydrive3dx.data.repository

import com.hypermob.mydrive3dx.data.mapper.toDomain
import com.hypermob.mydrive3dx.data.mapper.toDto
import com.hypermob.mydrive3dx.data.remote.api.BodyScanApi
import com.hypermob.mydrive3dx.data.remote.dto.BodyScanDataDto
import com.hypermob.mydrive3dx.domain.model.*
import com.hypermob.mydrive3dx.domain.repository.BodyScanRepository
import com.hypermob.mydrive3dx.domain.repository.BodyImageUploadResult
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.MultipartBody
import okhttp3.RequestBody.Companion.toRequestBody
import javax.inject.Inject
import javax.inject.Singleton

/**
 * Body Scan Repository Implementation
 */
@Singleton
class BodyScanRepositoryImpl @Inject constructor(
    private val bodyScanApi: BodyScanApi
) : BodyScanRepository {

    override suspend fun createProfile(
        userId: String,
        bodyScanData: BodyScan,
        vehicleId: String?
    ): Flow<Result<BodyProfile>> = flow {
        emit(Result.Loading)
        try {
            val requestDto = BodyScanDataDto(
                userId = userId,
                vehicleId = vehicleId,
                sittingHeight = bodyScanData.sittingHeight,
                shoulderWidth = bodyScanData.shoulderWidth,
                armLength = bodyScanData.armLength,
                headHeight = bodyScanData.headHeight,
                eyeHeight = bodyScanData.eyeHeight,
                legLength = bodyScanData.legLength,
                torsoLength = bodyScanData.torsoLength,
                weight = bodyScanData.weight,
                height = bodyScanData.height
            )

            val response = bodyScanApi.createProfile(requestDto)
            if (response.success == true && response.data != null) {
                val profile = response.data.toDomain()
                emit(Result.Success(profile))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to create profile")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun getProfile(
        userId: String,
        vehicleId: String?
    ): Flow<Result<BodyProfile>> = flow {
        emit(Result.Loading)
        try {
            val response = bodyScanApi.getProfile(userId, vehicleId)
            if (response.success == true && response.data != null) {
                val profile = response.data.toDomain()
                emit(Result.Success(profile))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to get profile")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun getAllProfiles(userId: String): Flow<Result<List<BodyProfile>>> = flow {
        emit(Result.Loading)
        try {
            val response = bodyScanApi.getAllProfiles(userId)
            if (response.success == true && response.data != null) {
                val profiles = response.data.map { it.toDomain() }
                emit(Result.Success(profiles))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to get profiles")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun setActiveProfile(profileId: String): Flow<Result<Unit>> = flow {
        emit(Result.Loading)
        try {
            val response = bodyScanApi.setActiveProfile(profileId)
            if (response.success == true) {
                emit(Result.Success(Unit))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to activate profile")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun deleteProfile(profileId: String): Flow<Result<Unit>> = flow {
        emit(Result.Loading)
        try {
            val response = bodyScanApi.deleteProfile(profileId)
            if (response.success == true) {
                emit(Result.Success(Unit))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to delete profile")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun applyVehicleSettings(
        profileId: String,
        vehicleId: String
    ): Flow<Result<VehicleSettingResult>> = flow {
        emit(Result.Loading)
        try {
            val response = bodyScanApi.applyVehicleSettings(profileId, vehicleId)
            if (response.success == true && response.data != null) {
                val result = response.data.toDomain()
                emit(Result.Success(result))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to apply settings")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun updateVehicleSettings(
        profileId: String,
        settings: VehicleSettings
    ): Flow<Result<BodyProfile>> = flow {
        emit(Result.Loading)
        try {
            val response = bodyScanApi.updateVehicleSettings(profileId, settings.toDto())
            if (response.success == true && response.data != null) {
                val profile = response.data.toDomain()
                emit(Result.Success(profile))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to update settings")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun uploadBodyImage(
        userId: String,
        imageBytes: ByteArray,
        height: Float?
    ): Flow<Result<BodyImageUploadResult>> = flow {
        emit(Result.Loading)
        try {
            // Create multipart body
            val imageRequestBody = imageBytes.toRequestBody("image/jpeg".toMediaType())
            val imagePart = MultipartBody.Part.createFormData(
                "image",
                "body_${System.currentTimeMillis()}.jpg",
                imageRequestBody
            )

            val userIdRequestBody = userId.toRequestBody("text/plain".toMediaType())

            val heightRequestBody = height?.let {
                it.toString().toRequestBody("text/plain".toMediaType())
            }

            val response = bodyScanApi.uploadBodyImage(imagePart, userIdRequestBody, heightRequestBody)

            val result = BodyImageUploadResult(
                success = response.body_image_url != null,
                s3Path = response.body_image_url,
                bodyDataId = null,
                message = response.message
            )

            if (response.body_image_url != null) {
                emit(Result.Success(result))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to upload body image")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun generateMeasurement(
        userId: String,
        imageS3Path: String,
        heightCm: Int
    ): Flow<Result<com.hypermob.mydrive3dx.domain.repository.MeasurementResult>> = flow {
        emit(Result.Loading)
        try {
            val request = com.hypermob.mydrive3dx.data.remote.api.MeasureRequest(
                user_id = userId,
                image_url = imageS3Path,
                height_cm = heightCm.toDouble()
            )

            val response = bodyScanApi.generateMeasurement(request)

            val data = response.data
            val result = com.hypermob.mydrive3dx.domain.repository.MeasurementResult(
                success = response.status == "success",
                measurementId = null,
                shoulderWidth = null,
                armLength = null,
                torsoHeight = data?.torso,
                legLength = null,
                bodyType = null,
                message = null,
                upperArmCm = data?.upper_arm,
                forearmCm = data?.forearm,
                thighCm = data?.thigh,
                calfCm = data?.calf,
                torsoCm = data?.torso
            )

            if (response.status == "success") {
                emit(Result.Success(result))
            } else {
                emit(Result.Error(Exception("Failed to generate measurement")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun generateRecommendation(
        userId: String,
        measurementId: String
    ): Flow<Result<com.hypermob.mydrive3dx.domain.repository.RecommendationResult>> = flow {
        emit(Result.Loading)
        emit(Result.Error(Exception("Use generateRecommendationWithMeasurements instead")))
    }

    override suspend fun generateRecommendationWithMeasurements(
        userId: String,
        height: Double,
        upperArm: Double,
        forearm: Double,
        thigh: Double,
        calf: Double,
        torso: Double
    ): Flow<Result<com.hypermob.mydrive3dx.domain.repository.RecommendationResult>> = flow {
        emit(Result.Loading)
        try {
            val request = com.hypermob.mydrive3dx.data.remote.api.RecommendRequest(
                user_id = userId,
                height = height,
                upper_arm = upperArm,
                forearm = forearm,
                thigh = thigh,
                calf = calf,
                torso = torso
            )

            val response = bodyScanApi.generateRecommendation(request)

            val settings = response.data
            val result = com.hypermob.mydrive3dx.domain.repository.RecommendationResult(
                success = response.status == "success",
                profileId = null,
                seatPosition = settings?.seat_position?.toInt(),
                seatHeight = settings?.seat_front_height?.toInt(),
                steeringWheelPosition = settings?.wheel_position?.toInt(),
                mirrorAngle = settings?.mirror_left_yaw?.toInt(),
                message = null
            )

            if (response.status == "success" && settings != null) {
                // Save profile to database automatically
                try {
                    val profileRequest = com.hypermob.mydrive3dx.data.remote.api.ProfileSaveRequest(
                        bodyMeasurements = com.hypermob.mydrive3dx.data.remote.api.ProfileBodyMeasurements(
                            height = height
                        ),
                        vehicleSettings = com.hypermob.mydrive3dx.data.remote.api.ProfileVehicleSettings(
                            seat_position = settings.seat_position?.toInt(),
                            seat_angle = settings.seat_angle?.toInt(),
                            seat_front_height = settings.seat_front_height?.toInt(),
                            seat_rear_height = settings.seat_rear_height?.toInt(),
                            mirror_left_yaw = settings.mirror_left_yaw?.toInt(),
                            mirror_left_pitch = settings.mirror_left_pitch?.toInt(),
                            mirror_right_yaw = settings.mirror_right_yaw?.toInt(),
                            mirror_right_pitch = settings.mirror_right_pitch?.toInt(),
                            mirror_room_yaw = settings.mirror_room_yaw?.toInt(),
                            mirror_room_pitch = settings.mirror_room_pitch?.toInt(),
                            wheel_position = settings.wheel_position?.toInt(),
                            wheel_angle = settings.wheel_angle?.toInt()
                        )
                    )
                    bodyScanApi.saveProfile(userId, profileRequest)
                } catch (e: Exception) {
                    // Log but don't fail - profile save is not critical
                    e.printStackTrace()
                }

                emit(Result.Success(result))
            } else {
                emit(Result.Error(Exception("Failed to generate recommendation")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }
}
