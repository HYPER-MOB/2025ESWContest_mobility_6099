package com.hypermob.mydrive3dx.domain.usecase

import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.repository.AuthRepository
import kotlinx.coroutines.flow.Flow
import javax.inject.Inject

/**
 * Logout UseCase
 *
 * 로그아웃 비즈니스 로직
 */
class LogoutUseCase @Inject constructor(
    private val authRepository: AuthRepository
) {
    suspend operator fun invoke(): Flow<Result<Unit>> {
        return authRepository.logout()
    }
}
