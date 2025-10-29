package com.hypermob.android_mvp.data.api

import com.hypermob.android_mvp.data.model.*
import okhttp3.MultipartBody
import retrofit2.Response
import retrofit2.http.*

/**
 * Retrofit API Service for HYPERMOB MFA Authentication
 */
interface AuthService {

    @GET("health")
    suspend fun healthCheck(): Response<HealthResponse>

    @Multipart
    @POST("auth/face")
    suspend fun registerFace(
        @Part image: MultipartBody.Part
    ): Response<FaceRegisterResponse>

    @POST("auth/nfc")
    suspend fun registerNfc(
        @Body request: NfcRegisterRequest
    ): Response<NfcRegisterResponse>

    @GET("auth/ble")
    suspend fun getBleHashkey(
        @Query("user_id") userId: String,
        @Query("car_id") carId: String = "CAR123"
    ): Response<BleHashkeyResponse>

    @GET("auth/session")
    suspend fun getAuthSession(
        @Query("car_id") carId: String,
        @Query("user_id") userId: String
    ): Response<AuthSessionResponse>

    @POST("auth/result")
    suspend fun reportAuthResult(
        @Body request: AuthResultRequest
    ): Response<AuthResultResponse>

    @POST("auth/nfc/verify")
    suspend fun verifyNfc(
        @Body request: NfcVerifyRequest
    ): Response<NfcVerifyResponse>
}
