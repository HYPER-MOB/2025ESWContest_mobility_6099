package com.hypermob.mydrive3dx.domain.usecase

import com.hypermob.mydrive3dx.domain.model.AuthToken
import com.hypermob.mydrive3dx.domain.model.LoginRequest
import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.repository.AuthRepository
import kotlinx.coroutines.flow.Flow
import javax.inject.Inject

/**
 * Login UseCase
 *
 * 로그인 비즈니스 로직
 */
class LoginUseCase @Inject constructor(
    private val authRepository: AuthRepository
) {
    suspend operator fun invoke(email: String, password: String): Flow<Result<AuthToken>> {
        // 유효성 검사
        if (email.isBlank()) {
            return kotlinx.coroutines.flow.flow {
                emit(Result.Error(Exception("이메일을 입력해주세요")))
            }
        }
        if (password.isBlank()) {
            return kotlinx.coroutines.flow.flow {
                emit(Result.Error(Exception("비밀번호를 입력해주세요")))
            }
        }
        if (!android.util.Patterns.EMAIL_ADDRESS.matcher(email).matches()) {
            return kotlinx.coroutines.flow.flow {
                emit(Result.Error(Exception("올바른 이메일 형식이 아닙니다")))
            }
        }

        val request = LoginRequest(email = email, password = password)
        return authRepository.login(request)
    }
}
