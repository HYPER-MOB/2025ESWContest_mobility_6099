package com.hypermob.mydrive3dx.data.repository.fake

import com.hypermob.mydrive3dx.domain.model.*
import com.hypermob.mydrive3dx.domain.repository.BodyScanRepository
import com.hypermob.mydrive3dx.domain.repository.BodyImageUploadResult
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import java.time.LocalDateTime
import java.time.format.DateTimeFormatter
import javax.inject.Inject
import javax.inject.Singleton

/**
 * Fake Body Scan Repository Implementation
 *
 * API가 준비되기 전 테스트용 Repository
 */
@Singleton
class FakeBodyScanRepositoryImpl @Inject constructor() : BodyScanRepository {

    private val dateFormatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss")

    // 인메모리 프로필 저장소
    private val profiles = mutableListOf<BodyProfile>()

    override suspend fun createProfile(
        userId: String,
        bodyScanData: BodyScan,
        vehicleId: String?
    ): Flow<Result<BodyProfile>> = flow {
        emit(Result.Loading)
        delay(1000) // 바디스캔 처리 시뮬레이션

        val newProfile = BodyProfile(
            profileId = "profile_${System.currentTimeMillis()}",
            userId = userId,
            userName = "김현대",
            bodyScan = bodyScanData,
            vehicleSettings = VehicleSettings(
                seatFrontBack = 50.0,
                seatUpDown = 45.0,
                seatBackAngle = 15.0,
                seatHeadrestHeight = 50.0,
                leftMirrorAngle = 45.0,
                rightMirrorAngle = 45.0,
                steeringWheelHeight = 50.0,
                steeringWheelDepth = 50.0
            ),
            isActive = true,
            createdAt = LocalDateTime.now().format(dateFormatter),
            updatedAt = LocalDateTime.now().format(dateFormatter)
        )

        profiles.add(newProfile)
        emit(Result.Success(newProfile))
    }

    override suspend fun getProfile(
        userId: String,
        vehicleId: String?
    ): Flow<Result<BodyProfile>> = flow {
        emit(Result.Loading)
        delay(300)

        val profile = profiles.find { it.userId == userId && it.isActive }
        if (profile != null) {
            emit(Result.Success(profile))
        } else {
            emit(Result.Error(Exception("프로필을 찾을 수 없습니다")))
        }
    }

    override suspend fun getAllProfiles(userId: String): Flow<Result<List<BodyProfile>>> = flow {
        emit(Result.Loading)
        delay(300)

        val userProfiles = profiles.filter { it.userId == userId }
        emit(Result.Success(userProfiles))
    }

    override suspend fun setActiveProfile(profileId: String): Flow<Result<Unit>> = flow {
        emit(Result.Loading)
        delay(200)

        // 모든 프로필 비활성화
        profiles.forEachIndexed { index, profile ->
            profiles[index] = profile.copy(isActive = false)
        }

        // 선택한 프로필만 활성화
        val profileIndex = profiles.indexOfFirst { it.profileId == profileId }
        if (profileIndex >= 0) {
            profiles[profileIndex] = profiles[profileIndex].copy(isActive = true)
            emit(Result.Success(Unit))
        } else {
            emit(Result.Error(Exception("프로필을 찾을 수 없습니다")))
        }
    }

    override suspend fun deleteProfile(profileId: String): Flow<Result<Unit>> = flow {
        emit(Result.Loading)
        delay(200)

        val removed = profiles.removeIf { it.profileId == profileId }
        if (removed) {
            emit(Result.Success(Unit))
        } else {
            emit(Result.Error(Exception("프로필을 찾을 수 없습니다")))
        }
    }

    override suspend fun applyVehicleSettings(
        profileId: String,
        vehicleId: String
    ): Flow<Result<VehicleSettingResult>> = flow {
        emit(Result.Loading)
        delay(500)

        val profile = profiles.find { it.profileId == profileId }
        if (profile != null) {
            emit(Result.Success(VehicleSettingResult(
                success = true,
                profileId = profileId,
                appliedSettings = profile.vehicleSettings,
                estimatedTime = 15,
                message = "차량 세팅이 적용되었습니다"
            )))
        } else {
            emit(Result.Error(Exception("프로필을 찾을 수 없습니다")))
        }
    }

    override suspend fun updateVehicleSettings(
        profileId: String,
        settings: VehicleSettings
    ): Flow<Result<BodyProfile>> = flow {
        emit(Result.Loading)
        delay(300)

        val profileIndex = profiles.indexOfFirst { it.profileId == profileId }
        if (profileIndex >= 0) {
            val updatedProfile = profiles[profileIndex].copy(vehicleSettings = settings)
            profiles[profileIndex] = updatedProfile
            emit(Result.Success(updatedProfile))
        } else {
            emit(Result.Error(Exception("프로필을 찾을 수 없습니다")))
        }
    }

    override suspend fun uploadBodyImage(
        userId: String,
        imageBytes: ByteArray,
        height: Float?
    ): Flow<Result<BodyImageUploadResult>> = flow {
        emit(Result.Loading)
        delay(1500) // 업로드 시뮬레이션

        // Fake 업로드 결과
        val result = BodyImageUploadResult(
            success = true,
            s3Path = "https://hypermob-images.s3.ap-northeast-2.amazonaws.com/body/${System.currentTimeMillis()}.jpg",
            bodyDataId = "body_data_${System.currentTimeMillis()}",
            message = "전신 사진이 성공적으로 업로드되었습니다"
        )

        emit(Result.Success(result))
    }

    override suspend fun generateMeasurement(
        userId: String,
        imageS3Path: String,
        heightCm: Int
    ): Flow<Result<com.hypermob.mydrive3dx.domain.repository.MeasurementResult>> = flow {
        emit(Result.Loading)
        delay(1500) // Simulate network delay

        val result = com.hypermob.mydrive3dx.domain.repository.MeasurementResult(
            success = true,
            measurementId = "measurement_${System.currentTimeMillis()}",
            shoulderWidth = 45.5,
            armLength = 62.3,
            torsoHeight = 55.8,
            legLength = 88.5,
            bodyType = "Normal",
            message = "체형 데이터가 성공적으로 생성되었습니다"
        )

        emit(Result.Success(result))
    }

    override suspend fun generateRecommendation(
        userId: String,
        measurementId: String
    ): Flow<Result<com.hypermob.mydrive3dx.domain.repository.RecommendationResult>> = flow {
        emit(Result.Loading)
        delay(2000) // Simulate network delay

        val result = com.hypermob.mydrive3dx.domain.repository.RecommendationResult(
            success = true,
            profileId = "profile_${System.currentTimeMillis()}",
            seatPosition = 120,
            seatHeight = 45,
            steeringWheelPosition = 70,
            mirrorAngle = 30,
            message = "맞춤형 차량 프로필이 생성되었습니다"
        )

        emit(Result.Success(result))
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
        delay(2000) // Simulate network delay

        val result = com.hypermob.mydrive3dx.domain.repository.RecommendationResult(
            success = true,
            profileId = "profile_${System.currentTimeMillis()}",
            seatPosition = 120,
            seatHeight = 45,
            steeringWheelPosition = 70,
            mirrorAngle = 30,
            message = "맞춤형 차량 프로필이 생성되었습니다"
        )

        emit(Result.Success(result))
    }
}
