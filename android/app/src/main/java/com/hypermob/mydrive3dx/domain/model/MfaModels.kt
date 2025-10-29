package com.hypermob.mydrive3dx.domain.model

import kotlinx.serialization.Serializable

/**
 * MFA Authentication Result
 * MFA 인증 결과
 */
data class MfaAuthenticationResult(
    val faceVerified: Boolean,
    val bleVerified: Boolean,
    val nfcVerified: Boolean,
    val serverVerified: Boolean,
    val token: String?
)

/**
 * MFA Enrollment Requests
 * MFA 등록 요청
 */
@Serializable
data class EnrollFaceRequest(
    val user_id: String,
    val face_token: String
)

@Serializable
data class EnrollBleRequest(
    val user_id: String,
    val device_id: String,
    val hash_key: String
)

@Serializable
data class EnrollNfcRequest(
    val user_id: String,
    val nfc_uid: String
)

/**
 * MFA Enrollment Response
 * MFA 등록 응답
 */
@Serializable
data class EnrollResponse(
    val success: Boolean,
    val message: String
)

/**
 * MFA Authentication Request
 * MFA 인증 요청
 */
@Serializable
data class MfaAuthRequest(
    val user_id: String,
    val face_token: String,
    val ble_hash_key: String,
    val nfc_uid: String
)

/**
 * MFA Authentication Response
 * MFA 인증 응답
 */
@Serializable
data class MfaAuthResponse(
    val success: Boolean,
    val token: String?,
    val message: String
)

/**
 * MFA Step Status
 * MFA 단계별 상태
 */
sealed class MfaStepStatus(val message: String) {
    object Pending : MfaStepStatus("Tap to start")
    object InProgress : MfaStepStatus("Authenticating...")
    object Completed : MfaStepStatus("Verified")
    data class Failed(val error: String) : MfaStepStatus(error)
}

/**
 * NFC Verify Request (android_mvp 호환)
 * POST /auth/nfc/verify
 */
@Serializable
data class NfcVerifyRequest(
    val user_id: String,
    val nfc_uid: String,
    val car_id: String
)

/**
 * NFC Verify Response (android_mvp 호환)
 */
@Serializable
data class NfcVerifyResponse(
    val status: String,  // "MFA_SUCCESS" or "MFA_FAILED"
    val message: String,
    val session_id: String? = null,
    val timestamp: String? = null
)

/**
 * Face Register Result (Domain Model)
 * 얼굴 등록 결과
 */
data class FaceRegisterResult(
    val userId: String,
    val faceId: String,
    val nfcUid: String,
    val status: String
)
