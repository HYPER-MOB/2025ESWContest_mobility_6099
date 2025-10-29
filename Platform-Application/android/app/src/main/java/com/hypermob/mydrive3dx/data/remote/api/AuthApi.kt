package com.hypermob.mydrive3dx.data.remote.api

import com.hypermob.mydrive3dx.data.remote.dto.*
import retrofit2.http.*

/**
 * Auth API Interface
 *
 * 인증 관련 API 엔드포인트
 */
interface AuthApi {

    /**
     * 로그인
     * POST /api/auth/login
     */
    @POST("auth/login")
    suspend fun login(
        @Body request: LoginRequestDto
    ): ApiResponse<AuthTokenDto>

    /**
     * 회원가입
     * POST /users
     */
    @POST("users")
    suspend fun register(
        @Body request: RegisterRequestDto
    ): UserRegisterResponse

    /**
     * 로그아웃
     * POST /api/auth/logout
     */
    @POST("auth/logout")
    suspend fun logout(): ApiResponse<Unit>

    /**
     * 토큰 갱신
     * POST /api/auth/refresh
     */
    @POST("auth/refresh")
    suspend fun refreshToken(
        @Body refreshToken: RefreshTokenRequestDto
    ): ApiResponse<AuthTokenDto>

    /**
     * 현재 사용자 정보 조회
     * GET /api/auth/me
     */
    @GET("auth/me")
    suspend fun getCurrentUser(): ApiResponse<UserDto>

    /**
     * 얼굴 등록 (android_mvp 호환)
     * POST /auth/face
     */
    @Multipart
    @POST("auth/face")
    suspend fun registerFace(
        @Part image: okhttp3.MultipartBody.Part
    ): FaceRegisterResponse

    /**
     * 얼굴 랜드마크 추출
     * POST /facedata
     */
    @POST("facedata")
    suspend fun extractFaceLandmarks(
        @Body request: FaceDataRequest
    ): FaceDataResponse
}

/**
 * Refresh Token Request DTO
 */
@kotlinx.serialization.Serializable
data class RefreshTokenRequestDto(
    val refreshToken: String
)

/**
 * User Register Response
 */
@kotlinx.serialization.Serializable
data class UserRegisterResponse(
    val user_id: String,
    val name: String,
    val email: String,
    val status: String
)

/**
 * Face Register Response (android_mvp 호환)
 */
@kotlinx.serialization.Serializable
data class FaceRegisterResponse(
    val user_id: String,
    val face_id: String? = null,
    val nfc_uid: String? = null,
    val face_image_url: String? = null,
    val status: String
)

/**
 * Face Data Request
 */
@kotlinx.serialization.Serializable
data class FaceDataRequest(
    val image_url: String,
    val user_id: String
)

/**
 * Face Data Response
 */
@kotlinx.serialization.Serializable
data class FaceDataResponse(
    val status: String,
    val data: FaceLandmarkData? = null
)

@kotlinx.serialization.Serializable
data class FaceLandmarkData(
    val face_data_id: String? = null
)
