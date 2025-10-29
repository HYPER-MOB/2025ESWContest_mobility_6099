package com.hypermob.mydrive3dx.data.repository

import com.hypermob.mydrive3dx.data.local.TokenManager
import com.hypermob.mydrive3dx.data.local.datasource.UserLocalDataSource
import com.hypermob.mydrive3dx.data.mapper.toDomain
import com.hypermob.mydrive3dx.data.mapper.toDto
import com.hypermob.mydrive3dx.data.remote.api.AuthApi
import com.hypermob.mydrive3dx.data.remote.api.RefreshTokenRequestDto
import com.hypermob.mydrive3dx.domain.model.*
import com.hypermob.mydrive3dx.domain.repository.AuthRepository
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import okhttp3.MediaType.Companion.toMediaTypeOrNull
import javax.inject.Inject
import javax.inject.Singleton

/**
 * Auth Repository Implementation
 *
 * AuthRepository 인터페이스 구현체
 * 오프라인 캐싱 지원
 */
@Singleton
class AuthRepositoryImpl @Inject constructor(
    private val authApi: AuthApi,
    private val tokenManager: TokenManager,
    private val userLocalDataSource: UserLocalDataSource
) : AuthRepository {

    override suspend fun login(request: LoginRequest): Flow<Result<AuthToken>> = flow {
        emit(Result.Loading)
        try {
            // TODO: Server doesn't have email/password auth yet
            // For now, accept any email/password for testing
            kotlinx.coroutines.delay(500) // Simulate network delay

            // Create a test token
            val testToken = AuthToken(
                accessToken = "test_access_token_${System.currentTimeMillis()}",
                refreshToken = "test_refresh_token_${System.currentTimeMillis()}",
                expiresIn = 3600
            )
            tokenManager.saveToken(testToken)
            emit(Result.Success(testToken))

            /* Original implementation - uncomment when server has email/password auth
            val response = authApi.login(request.toDto())
            if (response.success == true && response.data != null) {
                val token = response.data.toDomain()
                tokenManager.saveToken(token)
                emit(Result.Success(token))
            } else {
                emit(Result.Error(Exception(response.message ?: "Login failed")))
            }
            */
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun register(request: RegisterRequest): Flow<Result<User>> = flow {
        emit(Result.Loading)
        try {
            val response = authApi.register(request.toDto())
            if (response.status == "created") {
                val user = User(
                    userId = response.user_id,
                    email = response.email,
                    name = response.name,
                    phone = "",
                    drivingLicense = null,
                    profileImage = null,
                    createdAt = java.time.LocalDateTime.now().toString()
                )
                emit(Result.Success(user))
            } else {
                emit(Result.Error(Exception("Registration failed: ${response.status}")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun logout(): Flow<Result<Unit>> = flow {
        emit(Result.Loading)
        try {
            val response = authApi.logout()
            if (response.success == true) {
                tokenManager.clearToken()
                // Clear all cached data on logout
                userLocalDataSource.clearAllUsers()
                emit(Result.Success(Unit))
            } else {
                emit(Result.Error(Exception(response.message ?: "Logout failed")))
            }
        } catch (e: Exception) {
            // 서버 요청 실패해도 로컬 토큰과 캐시는 삭제
            tokenManager.clearToken()
            userLocalDataSource.clearAllUsers()
            emit(Result.Error(e))
        }
    }

    override suspend fun refreshToken(refreshToken: String): Flow<Result<AuthToken>> = flow {
        emit(Result.Loading)
        try {
            val response = authApi.refreshToken(RefreshTokenRequestDto(refreshToken))
            if (response.success == true && response.data != null) {
                val token = response.data.toDomain()
                tokenManager.saveToken(token)
                emit(Result.Success(token))
            } else {
                emit(Result.Error(Exception(response.message ?: "Token refresh failed")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun getCurrentUser(): Flow<Result<User>> = flow {
        emit(Result.Loading)
        try {
            // Try to fetch from remote
            val response = authApi.getCurrentUser()
            if (response.success == true && response.data != null) {
                val user = response.data.toDomain()
                // Cache the user
                userLocalDataSource.saveUser(user)
                emit(Result.Success(user))
            } else {
                // If API call succeeds but returns no data, use test user
                emit(Result.Success(getTestUser()))
            }
        } catch (e: Exception) {
            // If remote fails, use test user for development/testing
            // TODO: Implement JWT token parsing to extract userId for cache lookup
            emit(Result.Success(getTestUser()))
        }
    }

    /**
     * Test user for development/testing
     * TODO: Remove this when proper authentication is implemented
     */
    private fun getTestUser(): User {
        return User(
            userId = "user001",
            email = "test@hypermob.com",
            name = "테스트 사용자",
            phone = "010-1234-5678",
            drivingLicense = "12-345678-90",
            profileImage = null,
            createdAt = java.time.LocalDateTime.now().toString()
        )
    }

    override suspend fun saveToken(token: AuthToken) {
        tokenManager.saveToken(token)
    }

    override suspend fun getToken(): AuthToken? {
        return tokenManager.getToken()
    }

    override suspend fun clearToken() {
        tokenManager.clearToken()
    }

    override fun isLoggedIn(): Flow<Boolean> {
        return tokenManager.isLoggedIn()
    }

    override suspend fun registerFace(imageBytes: ByteArray): Flow<Result<FaceRegisterResult>> = flow {
        emit(Result.Loading)
        try {
            // Step 1: Upload face image
            val requestBody = okhttp3.RequestBody.create(
                "image/jpeg".toMediaTypeOrNull(),
                imageBytes
            )
            val imagePart = okhttp3.MultipartBody.Part.createFormData(
                "image",
                "face_${System.currentTimeMillis()}.jpg",
                requestBody
            )

            val uploadResponse = authApi.registerFace(imagePart)

            // Step 2: Extract face landmarks
            if (uploadResponse.face_image_url != null) {
                val faceDataRequest = com.hypermob.mydrive3dx.data.remote.api.FaceDataRequest(
                    image_url = uploadResponse.face_image_url,
                    user_id = uploadResponse.user_id
                )
                val faceDataResponse = authApi.extractFaceLandmarks(faceDataRequest)

                // Save userId to TokenManager
                tokenManager.saveUserId(uploadResponse.user_id)
            }

            // Convert to domain model
            val result = FaceRegisterResult(
                userId = uploadResponse.user_id,
                faceId = uploadResponse.face_id ?: "",
                nfcUid = uploadResponse.nfc_uid ?: "",
                status = uploadResponse.status
            )

            // Save MFA info to TokenManager (if available)
            if (uploadResponse.face_id != null && uploadResponse.nfc_uid != null) {
                tokenManager.saveMfaInfo(
                    userId = uploadResponse.user_id,
                    faceId = uploadResponse.face_id,
                    nfcUid = uploadResponse.nfc_uid
                )
            }

            emit(Result.Success(result))
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }
}
