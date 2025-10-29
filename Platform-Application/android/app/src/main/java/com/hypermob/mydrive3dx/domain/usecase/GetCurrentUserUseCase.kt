package com.hypermob.mydrive3dx.domain.usecase

import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.model.User
import com.hypermob.mydrive3dx.domain.repository.AuthRepository
import kotlinx.coroutines.flow.Flow
import javax.inject.Inject

/**
 * Get Current User UseCase
 *
 * 현재 로그인한 사용자 정보 조회
 */
class GetCurrentUserUseCase @Inject constructor(
    private val authRepository: AuthRepository
) {
    suspend operator fun invoke(): Flow<Result<User>> {
        return authRepository.getCurrentUser()
    }
}
