package com.hypermob.mydrive3dx.domain.repository

import com.hypermob.mydrive3dx.domain.model.*

/**
 * MFA Repository Interface
 * MFA 인증 관련 데이터 저장소 인터페이스
 */
interface MfaRepository {
    suspend fun enrollFace(request: EnrollFaceRequest): EnrollResponse
    suspend fun enrollBle(request: EnrollBleRequest): EnrollResponse
    suspend fun enrollNfc(request: EnrollNfcRequest): EnrollResponse
    suspend fun authenticateMfa(faceToken: String, bleHashKey: String, nfcUid: String): MfaAuthResponse

    /**
     * NFC 인증 검증 (최종 MFA 검증)
     * android_mvp 호환 - POST /auth/nfc/verify
     */
    suspend fun verifyNfc(request: NfcVerifyRequest): NfcVerifyResponse
}
