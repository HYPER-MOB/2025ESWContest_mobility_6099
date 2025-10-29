package com.hypermob.mydrive3dx.domain.repository

import com.hypermob.mydrive3dx.domain.model.*
import kotlinx.coroutines.flow.Flow

/**
 * Auth Repository Interface
 *
 * 인증 관련 Repository 인터페이스 (Domain Layer)
 */
interface AuthRepository {

    /**
     * 로그인
     */
    suspend fun login(request: LoginRequest): Flow<Result<AuthToken>>

    /**
     * 회원가입
     */
    suspend fun register(request: RegisterRequest): Flow<Result<User>>

    /**
     * 로그아웃
     */
    suspend fun logout(): Flow<Result<Unit>>

    /**
     * 토큰 갱신
     */
    suspend fun refreshToken(refreshToken: String): Flow<Result<AuthToken>>

    /**
     * 현재 사용자 정보 조회
     */
    suspend fun getCurrentUser(): Flow<Result<User>>

    /**
     * 토큰 저장
     */
    suspend fun saveToken(token: AuthToken)

    /**
     * 토큰 조회
     */
    suspend fun getToken(): AuthToken?

    /**
     * 토큰 삭제
     */
    suspend fun clearToken()

    /**
     * 로그인 상태 확인
     */
    fun isLoggedIn(): Flow<Boolean>

    /**
     * 얼굴 등록 (MFA)
     * android_mvp 호환
     */
    suspend fun registerFace(imageBytes: ByteArray): Flow<Result<FaceRegisterResult>>
}
