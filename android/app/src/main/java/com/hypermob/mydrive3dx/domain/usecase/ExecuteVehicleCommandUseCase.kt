package com.hypermob.mydrive3dx.domain.usecase

import com.hypermob.mydrive3dx.domain.model.ControlCommandResponse
import com.hypermob.mydrive3dx.domain.model.ControlCommandType
import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.repository.VehicleControlRepository
import kotlinx.coroutines.flow.Flow
import javax.inject.Inject

/**
 * Execute Vehicle Command UseCase
 *
 * 차량 제어 명령 실행
 */
class ExecuteVehicleCommandUseCase @Inject constructor(
    private val vehicleControlRepository: VehicleControlRepository
) {
    suspend fun lockDoors(rentalId: String): Flow<Result<ControlCommandResponse>> {
        return vehicleControlRepository.lockDoors(rentalId)
    }

    suspend fun unlockDoors(rentalId: String): Flow<Result<ControlCommandResponse>> {
        return vehicleControlRepository.unlockDoors(rentalId)
    }

    suspend fun startEngine(rentalId: String): Flow<Result<ControlCommandResponse>> {
        return vehicleControlRepository.startEngine(rentalId)
    }

    suspend fun stopEngine(rentalId: String): Flow<Result<ControlCommandResponse>> {
        return vehicleControlRepository.stopEngine(rentalId)
    }

    suspend fun turnOnClimate(rentalId: String, temperature: Double): Flow<Result<ControlCommandResponse>> {
        return vehicleControlRepository.turnOnClimate(rentalId, temperature)
    }

    suspend fun turnOffClimate(rentalId: String): Flow<Result<ControlCommandResponse>> {
        return vehicleControlRepository.turnOffClimate(rentalId)
    }

    suspend fun honkHorn(rentalId: String): Flow<Result<ControlCommandResponse>> {
        return vehicleControlRepository.honkHorn(rentalId)
    }

    suspend fun flashLights(rentalId: String): Flow<Result<ControlCommandResponse>> {
        return vehicleControlRepository.flashLights(rentalId)
    }
}
