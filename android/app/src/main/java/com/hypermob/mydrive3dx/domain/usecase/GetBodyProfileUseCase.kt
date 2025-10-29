package com.hypermob.mydrive3dx.domain.usecase

import com.hypermob.mydrive3dx.domain.model.BodyProfile
import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.repository.BodyScanRepository
import kotlinx.coroutines.flow.Flow
import javax.inject.Inject

/**
 * Get Body Profile UseCase
 *
 * 사용자의 바디 프로필 조회
 */
class GetBodyProfileUseCase @Inject constructor(
    private val bodyScanRepository: BodyScanRepository
) {
    suspend operator fun invoke(
        userId: String,
        vehicleId: String? = null
    ): Flow<Result<BodyProfile>> {
        return bodyScanRepository.getProfile(userId, vehicleId)
    }
}
