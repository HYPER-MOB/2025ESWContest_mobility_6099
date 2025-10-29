package com.hypermob.mydrive3dx.domain.usecase

import com.hypermob.mydrive3dx.domain.model.RegisterRequest
import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.model.User
import com.hypermob.mydrive3dx.domain.repository.AuthRepository
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import javax.inject.Inject

/**
 * Register UseCase
 *
 * 회원가입 비즈니스 로직
 */
class RegisterUseCase @Inject constructor(
    private val authRepository: AuthRepository
) {
    suspend operator fun invoke(
        email: String,
        password: String,
        confirmPassword: String,
        name: String,
        phone: String,
        drivingLicense: String?
    ): Flow<Result<User>> {
        // 유효성 검사
        if (email.isBlank()) {
            return flow { emit(Result.Error(Exception("이메일을 입력해주세요"))) }
        }
        if (!android.util.Patterns.EMAIL_ADDRESS.matcher(email).matches()) {
            return flow { emit(Result.Error(Exception("올바른 이메일 형식이 아닙니다"))) }
        }
        if (password.isBlank()) {
            return flow { emit(Result.Error(Exception("비밀번호를 입력해주세요"))) }
        }
        if (password.length < 8) {
            return flow { emit(Result.Error(Exception("비밀번호는 8자 이상이어야 합니다"))) }
        }
        if (password != confirmPassword) {
            return flow { emit(Result.Error(Exception("비밀번호가 일치하지 않습니다"))) }
        }
        if (name.isBlank()) {
            return flow { emit(Result.Error(Exception("이름을 입력해주세요"))) }
        }
        if (phone.isBlank()) {
            return flow { emit(Result.Error(Exception("전화번호를 입력해주세요"))) }
        }

        val request = RegisterRequest(
            email = email,
            password = password,
            name = name,
            phone = phone,
            drivingLicense = drivingLicense
        )
        return authRepository.register(request)
    }
}
