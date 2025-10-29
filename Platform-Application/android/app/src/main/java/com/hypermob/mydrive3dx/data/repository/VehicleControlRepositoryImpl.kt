package com.hypermob.mydrive3dx.data.repository

import com.hypermob.mydrive3dx.data.mapper.toDomain
import com.hypermob.mydrive3dx.data.mapper.toDto
import com.hypermob.mydrive3dx.data.remote.api.VehicleControlApi
import com.hypermob.mydrive3dx.data.remote.dto.ControlCommandRequestDto
import com.hypermob.mydrive3dx.domain.model.*
import com.hypermob.mydrive3dx.domain.repository.VehicleControlRepository
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import javax.inject.Inject
import javax.inject.Singleton

/**
 * Vehicle Control Repository Implementation
 */
@Singleton
class VehicleControlRepositoryImpl @Inject constructor(
    private val vehicleControlApi: VehicleControlApi
) : VehicleControlRepository {

    override suspend fun getVehicleControl(rentalId: String): Flow<Result<VehicleControl>> = flow {
        emit(Result.Loading)
        try {
            val response = vehicleControlApi.getVehicleControl(rentalId)
            if (response.success == true && response.data != null) {
                val control = response.data.toDomain()
                emit(Result.Success(control))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to get vehicle control")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun lockDoors(rentalId: String): Flow<Result<ControlCommandResponse>> = flow {
        emit(Result.Loading)
        try {
            val request = ControlCommandRequestDto(
                rentalId = rentalId,
                commandType = ControlCommandType.LOCK_DOORS.name,
                parameters = null
            )
            val response = vehicleControlApi.executeCommand(rentalId, request)
            if (response.success == true && response.data != null) {
                emit(Result.Success(response.data.toDomain()))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to lock doors")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun unlockDoors(rentalId: String): Flow<Result<ControlCommandResponse>> = flow {
        emit(Result.Loading)
        try {
            val request = ControlCommandRequestDto(
                rentalId = rentalId,
                commandType = ControlCommandType.UNLOCK_DOORS.name,
                parameters = null
            )
            val response = vehicleControlApi.executeCommand(rentalId, request)
            if (response.success == true && response.data != null) {
                emit(Result.Success(response.data.toDomain()))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to unlock doors")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun startEngine(rentalId: String): Flow<Result<ControlCommandResponse>> = flow {
        emit(Result.Loading)
        try {
            val request = ControlCommandRequestDto(
                rentalId = rentalId,
                commandType = ControlCommandType.START_ENGINE.name,
                parameters = null
            )
            val response = vehicleControlApi.executeCommand(rentalId, request)
            if (response.success == true && response.data != null) {
                emit(Result.Success(response.data.toDomain()))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to start engine")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun stopEngine(rentalId: String): Flow<Result<ControlCommandResponse>> = flow {
        emit(Result.Loading)
        try {
            val request = ControlCommandRequestDto(
                rentalId = rentalId,
                commandType = ControlCommandType.STOP_ENGINE.name,
                parameters = null
            )
            val response = vehicleControlApi.executeCommand(rentalId, request)
            if (response.success == true && response.data != null) {
                emit(Result.Success(response.data.toDomain()))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to stop engine")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun turnOnClimate(rentalId: String, temperature: Double): Flow<Result<ControlCommandResponse>> = flow {
        emit(Result.Loading)
        try {
            val request = ControlCommandRequestDto(
                rentalId = rentalId,
                commandType = ControlCommandType.TURN_ON_CLIMATE.name,
                parameters = mapOf("temperature" to temperature.toString())
            )
            val response = vehicleControlApi.executeCommand(rentalId, request)
            if (response.success == true && response.data != null) {
                emit(Result.Success(response.data.toDomain()))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to turn on climate")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun turnOffClimate(rentalId: String): Flow<Result<ControlCommandResponse>> = flow {
        emit(Result.Loading)
        try {
            val request = ControlCommandRequestDto(
                rentalId = rentalId,
                commandType = ControlCommandType.TURN_OFF_CLIMATE.name,
                parameters = null
            )
            val response = vehicleControlApi.executeCommand(rentalId, request)
            if (response.success == true && response.data != null) {
                emit(Result.Success(response.data.toDomain()))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to turn off climate")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun honkHorn(rentalId: String): Flow<Result<ControlCommandResponse>> = flow {
        emit(Result.Loading)
        try {
            val request = ControlCommandRequestDto(
                rentalId = rentalId,
                commandType = ControlCommandType.HONK_HORN.name,
                parameters = null
            )
            val response = vehicleControlApi.executeCommand(rentalId, request)
            if (response.success == true && response.data != null) {
                emit(Result.Success(response.data.toDomain()))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to honk horn")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun flashLights(rentalId: String): Flow<Result<ControlCommandResponse>> = flow {
        emit(Result.Loading)
        try {
            val request = ControlCommandRequestDto(
                rentalId = rentalId,
                commandType = ControlCommandType.FLASH_LIGHTS.name,
                parameters = null
            )
            val response = vehicleControlApi.executeCommand(rentalId, request)
            if (response.success == true && response.data != null) {
                emit(Result.Success(response.data.toDomain()))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to flash lights")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun executeCommand(request: ControlCommandRequest): Flow<Result<ControlCommandResponse>> = flow {
        emit(Result.Loading)
        try {
            val response = vehicleControlApi.executeCommand(request.rentalId, request.toDto())
            if (response.success == true && response.data != null) {
                emit(Result.Success(response.data.toDomain()))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to execute command")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }
}
