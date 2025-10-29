package com.hypermob.mydrive3dx.domain.usecase

import com.hypermob.mydrive3dx.domain.model.BodyProfile
import com.hypermob.mydrive3dx.domain.model.BodyScan
import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.repository.BodyScanRepository
import kotlinx.coroutines.flow.Flow
import javax.inject.Inject

/**
 * Create Body Profile UseCase
 *
 * 바디스캔 데이터로 프로필 생성
 */
class CreateBodyProfileUseCase @Inject constructor(
    private val bodyScanRepository: BodyScanRepository
) {
    suspend operator fun invoke(
        userId: String,
        bodyScanData: BodyScan,
        vehicleId: String? = null
    ): Flow<Result<BodyProfile>> {
        return bodyScanRepository.createProfile(userId, bodyScanData, vehicleId)
    }
}
