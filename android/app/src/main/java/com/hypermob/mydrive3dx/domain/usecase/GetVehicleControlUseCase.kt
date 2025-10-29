package com.hypermob.mydrive3dx.domain.usecase

import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.model.VehicleControl
import com.hypermob.mydrive3dx.domain.repository.VehicleControlRepository
import kotlinx.coroutines.flow.Flow
import javax.inject.Inject

/**
 * Get Vehicle Control UseCase
 *
 * 차량 제어 상태 조회
 */
class GetVehicleControlUseCase @Inject constructor(
    private val vehicleControlRepository: VehicleControlRepository
) {
    suspend operator fun invoke(rentalId: String): Flow<Result<VehicleControl>> {
        return vehicleControlRepository.getVehicleControl(rentalId)
    }
}
