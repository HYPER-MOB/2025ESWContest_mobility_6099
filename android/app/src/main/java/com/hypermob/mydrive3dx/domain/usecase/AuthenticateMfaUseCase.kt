package com.hypermob.mydrive3dx.domain.usecase

import com.hypermob.mydrive3dx.data.local.TokenManager
import com.hypermob.mydrive3dx.domain.model.MfaAuthenticationResult
import com.hypermob.mydrive3dx.domain.model.NfcVerifyRequest
import com.hypermob.mydrive3dx.domain.repository.MfaRepository
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import javax.inject.Inject

/**
 * MFA Authentication Use Case
 * MFA 인증 플로우 처리
 *
 * android_mvp 로직 기반:
 * 1. Face Registration (FaceRegisterActivity) → server returns user_id, face_id, nfc_uid
 * 2. BLE Connection (AuthenticationActivity) → write "ACCESS" to characteristic
 * 3. NFC Tag Read (NfcAuthActivity) → read UID from tag
 * 4. Server Verification (POST /auth/nfc/verify) → final MFA check
 */
class AuthenticateMfaUseCase @Inject constructor(
    private val mfaRepository: MfaRepository,
    private val tokenManager: TokenManager
) {

    /**
     * MFA 최종 검증
     * android_mvp의 NfcAuthActivity.kt 로직 기반
     *
     * @param userId 사용자 ID (TokenManager에서 가져옴)
     * @param nfcUid NFC 태그 UID
     * @param carId 차량 ID
     */
    suspend operator fun invoke(
        userId: String,
        nfcUid: String,
        carId: String
    ): Flow<MfaAuthenticationResult> = flow {

        try {
            // POST /auth/nfc/verify 호출
            val request = NfcVerifyRequest(
                user_id = userId,
                nfc_uid = nfcUid,
                car_id = carId
            )

            val response = mfaRepository.verifyNfc(request)

            // 서버 응답에 따라 결과 생성
            val success = response.status == "MFA_SUCCESS"

            emit(MfaAuthenticationResult(
                faceVerified = success,  // 서버에서 검증 완료
                bleVerified = success,   // 서버에서 검증 완료
                nfcVerified = success,   // 서버에서 검증 완료
                serverVerified = success,
                token = response.session_id  // session_id를 token으로 사용
            ))
        } catch (e: Exception) {
            // 에러 발생 시 실패 결과 emit
            emit(MfaAuthenticationResult(
                faceVerified = false,
                bleVerified = false,
                nfcVerified = false,
                serverVerified = false,
                token = null
            ))
            throw e
        }
    }
}
