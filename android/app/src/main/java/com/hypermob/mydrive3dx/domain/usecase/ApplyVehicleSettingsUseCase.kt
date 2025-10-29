package com.hypermob.mydrive3dx.domain.usecase

import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.model.VehicleSettingResult
import com.hypermob.mydrive3dx.domain.repository.BodyScanRepository
import kotlinx.coroutines.flow.Flow
import javax.inject.Inject

/**
 * Apply Vehicle Settings UseCase
 *
 * 차량에 프로필 기반 세팅 적용
 */
class ApplyVehicleSettingsUseCase @Inject constructor(
    private val bodyScanRepository: BodyScanRepository
) {
    suspend operator fun invoke(
        profileId: String,
        vehicleId: String
    ): Flow<Result<VehicleSettingResult>> {
        return bodyScanRepository.applyVehicleSettings(profileId, vehicleId)
    }
}
