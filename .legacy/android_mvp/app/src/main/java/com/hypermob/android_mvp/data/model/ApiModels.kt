package com.hypermob.android_mvp.data.model

import com.google.gson.annotations.SerializedName

/**
 * API Response Models for HYPERMOB MFA Authentication
 */

// POST /auth/face
data class FaceRegisterRequest(
    val image: ByteArray // Will be sent as multipart/form-data
)

data class FaceRegisterResponse(
    @SerializedName("user_id") val userId: String,
    @SerializedName("face_id") val faceId: String,
    @SerializedName("nfc_uid") val nfcUid: String,
    val status: String
)

// POST /auth/nfc
data class NfcRegisterRequest(
    @SerializedName("user_id") val userId: String,
    @SerializedName("nfc_uid") val nfcUid: String
)

data class NfcRegisterResponse(
    val status: String
)

// GET /auth/ble
data class BleHashkeyResponse(
    val hashkey: String,
    @SerializedName("expires_at") val expiresAt: String
)

// GET /auth/session
data class AuthSessionResponse(
    @SerializedName("session_id") val sessionId: String,
    @SerializedName("face_id") val faceId: String,
    val hashkey: String,
    @SerializedName("nfc_uid") val nfcUid: String,
    val status: String
)

// POST /auth/result
data class AuthResultRequest(
    @SerializedName("session_id") val sessionId: String,
    @SerializedName("car_id") val carId: String,
    @SerializedName("face_verified") val faceVerified: Boolean,
    @SerializedName("ble_verified") val bleVerified: Boolean,
    @SerializedName("nfc_verified") val nfcVerified: Boolean
)

data class AuthResultResponse(
    val status: String, // MFA_SUCCESS or MFA_FAILED
    val message: String,
    @SerializedName("session_id") val sessionId: String,
    val timestamp: String,
    @SerializedName("failed_steps") val failedSteps: List<String>? = null
)

// POST /auth/nfc/verify
data class NfcVerifyRequest(
    @SerializedName("user_id") val userId: String,
    @SerializedName("nfc_uid") val nfcUid: String,
    @SerializedName("car_id") val carId: String
)

data class NfcVerifyResponse(
    val status: String, // MFA_SUCCESS or MFA_FAILED
    val message: String,
    @SerializedName("session_id") val sessionId: String,
    val timestamp: String
)

// GET /health
data class HealthResponse(
    val status: String,
    val timestamp: String,
    val database: String
)

// Error Response
data class ErrorResponse(
    val error: String,
    val message: String,
    val details: Map<String, Any>? = null
)
