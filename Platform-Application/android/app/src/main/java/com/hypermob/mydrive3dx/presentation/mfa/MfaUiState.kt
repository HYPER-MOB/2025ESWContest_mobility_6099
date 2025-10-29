package com.hypermob.mydrive3dx.presentation.mfa

/**
 * MFA UI State
 *
 * MFA 인증 화면의 상태
 */
data class MfaUiState(
    val rentalId: String? = null,
    val faceVerified: Boolean = false,
    val nfcVerified: Boolean = false,
    val bleVerified: Boolean = false,
    val isLoading: Boolean = false,
    val isComplete: Boolean = false,
    val errorMessage: String? = null,
    // 시나리오3 - Polling 후 자동 전환
    val shouldNavigateToControl: Boolean = false
) {
    val allVerified: Boolean
        get() = faceVerified && nfcVerified && bleVerified
}
