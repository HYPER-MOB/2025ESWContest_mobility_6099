package com.hypermob.mydrive3dx.data.remote.api

import com.hypermob.mydrive3dx.domain.model.*
import retrofit2.http.*

/**
 * MFA API Interface
 * MFA (Multi-Factor Authentication) 관련 API
 */
interface MfaApi {

    /**
     * POST /mfa/enroll/face
     * 얼굴 인식 등록
     */
    @POST("mfa/enroll/face")
    suspend fun enrollFace(
        @Body request: EnrollFaceRequest
    ): EnrollResponse

    /**
     * POST /mfa/enroll/ble
     * BLE 기기 등록
     */
    @POST("mfa/enroll/ble")
    suspend fun enrollBle(
        @Body request: EnrollBleRequest
    ): EnrollResponse

    /**
     * POST /mfa/enroll/nfc
     * NFC 카드 등록
     */
    @POST("mfa/enroll/nfc")
    suspend fun enrollNfc(
        @Body request: EnrollNfcRequest
    ): EnrollResponse

    /**
     * POST /mfa/authenticate
     * MFA 인증
     */
    @POST("mfa/authenticate")
    suspend fun authenticate(
        @Body request: MfaAuthRequest
    ): MfaAuthResponse

    /**
     * POST /auth/nfc/verify
     * NFC 인증 검증 (최종 MFA 검증)
     * android_mvp와 동일한 엔드포인트
     */
    @POST("auth/nfc/verify")
    suspend fun verifyNfc(
        @Body request: NfcVerifyRequest
    ): NfcVerifyResponse

    /**
     * GET /auth/result
     * MFA 인증 결과 조회 (시나리오3 - Polling용)
     */
    @GET("auth/result")
    suspend fun getAuthResult(
        @Query("user_id") userId: String,
        @Query("rental_id") rentalId: String? = null
    ): AuthResultResponse
}

/**
 * Auth Result Response (시나리오3)
 */
@kotlinx.serialization.Serializable
data class AuthResultResponse(
    val face_verified: Boolean = false,
    val nfc_verified: Boolean = false,
    val ble_verified: Boolean = false,
    val all_verified: Boolean = false,
    val message: String? = null
)
