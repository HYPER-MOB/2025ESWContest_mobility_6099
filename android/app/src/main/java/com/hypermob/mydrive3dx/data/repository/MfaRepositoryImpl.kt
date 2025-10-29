package com.hypermob.mydrive3dx.data.repository

import com.hypermob.mydrive3dx.data.remote.api.MfaApi
import com.hypermob.mydrive3dx.domain.model.*
import com.hypermob.mydrive3dx.domain.repository.MfaRepository
import javax.inject.Inject

/**
 * MFA Repository Implementation
 * MFA 인증 관련 데이터 저장소 구현
 */
class MfaRepositoryImpl @Inject constructor(
    private val api: MfaApi
) : MfaRepository {

    override suspend fun enrollFace(request: EnrollFaceRequest): EnrollResponse {
        return api.enrollFace(request)
    }

    override suspend fun enrollBle(request: EnrollBleRequest): EnrollResponse {
        return api.enrollBle(request)
    }

    override suspend fun enrollNfc(request: EnrollNfcRequest): EnrollResponse {
        return api.enrollNfc(request)
    }

    override suspend fun authenticateMfa(
        faceToken: String,
        bleHashKey: String,
        nfcUid: String
    ): MfaAuthResponse {
        // TODO: user_id는 현재 로그인한 사용자 ID를 사용해야 함
        val userId = "current_user_id" // 임시값
        val request = MfaAuthRequest(
            user_id = userId,
            face_token = faceToken,
            ble_hash_key = bleHashKey,
            nfc_uid = nfcUid
        )
        return api.authenticate(request)
    }

    override suspend fun verifyNfc(request: NfcVerifyRequest): NfcVerifyResponse {
        return api.verifyNfc(request)
    }
}
