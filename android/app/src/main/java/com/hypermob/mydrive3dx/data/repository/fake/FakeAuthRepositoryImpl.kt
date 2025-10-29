package com.hypermob.mydrive3dx.data.repository.fake

import com.hypermob.mydrive3dx.data.local.TokenManager
import com.hypermob.mydrive3dx.domain.model.*
import com.hypermob.mydrive3dx.domain.repository.AuthRepository
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.flow.flowOf
import javax.inject.Inject
import javax.inject.Singleton

/**
 * Fake Auth Repository Implementation
 *
 * API가 준비되기 전 테스트용 Repository
 */
@Singleton
class FakeAuthRepositoryImpl @Inject constructor(
    private val tokenManager: TokenManager
) : AuthRepository {

    override suspend fun login(request: LoginRequest): Flow<Result<AuthToken>> = flow {
        emit(Result.Loading)
        delay(500) // 네트워크 지연 시뮬레이션

        // 간단한 로그인 검증 (실제로는 서버에서 처리)
        if (request.email.isNotEmpty() && request.password.isNotEmpty()) {
            tokenManager.saveToken(DummyData.dummyAuthToken)
            emit(Result.Success(DummyData.dummyAuthToken))
        } else {
            emit(Result.Error(Exception("이메일과 비밀번호를 입력해주세요")))
        }
    }

    override suspend fun register(request: RegisterRequest): Flow<Result<User>> = flow {
        emit(Result.Loading)
        delay(500)

        if (request.email.isNotEmpty() && request.password.isNotEmpty() && request.name.isNotEmpty()) {
            emit(Result.Success(DummyData.dummyUser))
        } else {
            emit(Result.Error(Exception("모든 필드를 입력해주세요")))
        }
    }

    override suspend fun logout(): Flow<Result<Unit>> = flow {
        emit(Result.Loading)
        delay(300)
        tokenManager.clearToken()
        emit(Result.Success(Unit))
    }

    override suspend fun refreshToken(refreshToken: String): Flow<Result<AuthToken>> = flow {
        emit(Result.Loading)
        delay(300)

        val newToken = DummyData.dummyAuthToken
        tokenManager.saveToken(newToken)
        emit(Result.Success(newToken))
    }

    override suspend fun getCurrentUser(): Flow<Result<User>> = flow {
        emit(Result.Loading)
        delay(300)

        val token = tokenManager.getToken()
        if (token != null) {
            emit(Result.Success(DummyData.dummyUser))
        } else {
            emit(Result.Error(Exception("로그인이 필요합니다")))
        }
    }

    override suspend fun saveToken(token: AuthToken) {
        tokenManager.saveToken(token)
    }

    override suspend fun getToken(): AuthToken? {
        return tokenManager.getToken()
    }

    override suspend fun clearToken() {
        tokenManager.clearToken()
    }

    override fun isLoggedIn(): Flow<Boolean> {
        return tokenManager.isLoggedIn()
    }

    override suspend fun registerFace(imageBytes: ByteArray): Flow<Result<FaceRegisterResult>> = flow {
        emit(Result.Loading)
        delay(1000) // 네트워크 지연 시뮬레이션

        // 더미 데이터 생성
        val result = FaceRegisterResult(
            userId = "fake_user_${System.currentTimeMillis()}",
            faceId = "fake_face_${System.currentTimeMillis()}",
            nfcUid = "AABBCCDD",  // 14자 hex (7바이트)
            status = "success"
        )

        // TokenManager에 저장
        tokenManager.saveMfaInfo(result.userId, result.faceId, result.nfcUid)

        emit(Result.Success(result))
    }
}
