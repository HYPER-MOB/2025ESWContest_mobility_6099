package com.hypermob.mydrive3dx.data.mapper

import com.hypermob.mydrive3dx.data.remote.dto.*
import com.hypermob.mydrive3dx.domain.model.*

/**
 * Vehicle Control Mapper
 *
 * DTO ↔ Domain Model 변환
 */

// DTO -> Domain
fun VehicleControlDto.toDomain(): VehicleControl {
    return VehicleControl(
        vehicleId = vehicleId,
        rentalId = rentalId,
        status = status.toDomain(),
        doors = doors.toDomain(),
        engine = engine.toDomain(),
        climate = climate.toDomain(),
        location = location?.toDomain(),
        lastUpdated = lastUpdated
    )
}

fun VehicleControlStatusDto.toDomain(): VehicleControlStatus {
    return VehicleControlStatus(
        isOnline = isOnline,
        batteryLevel = batteryLevel,
        fuelLevel = fuelLevel,
        odometer = odometer
    )
}

fun DoorStatusDto.toDomain(): DoorStatus {
    return DoorStatus(
        frontLeft = frontLeft,
        frontRight = frontRight,
        rearLeft = rearLeft,
        rearRight = rearRight,
        trunk = trunk
    )
}

fun EngineStatusDto.toDomain(): EngineStatus {
    return EngineStatus(
        isRunning = isRunning,
        temperature = temperature,
        rpm = rpm
    )
}

fun ClimateStatusDto.toDomain(): ClimateStatus {
    return ClimateStatus(
        isOn = isOn,
        temperature = temperature,
        fanSpeed = fanSpeed
    )
}

fun VehicleLocationDto.toDomain(): VehicleLocation {
    return VehicleLocation(
        latitude = latitude,
        longitude = longitude,
        address = address
    )
}

fun ControlCommandResponseDto.toDomain(): ControlCommandResponse {
    return ControlCommandResponse(
        success = success,
        message = message,
        updatedControl = updatedControl?.toDomain()
    )
}

// Domain -> DTO
fun ControlCommandRequest.toDto(): ControlCommandRequestDto {
    return ControlCommandRequestDto(
        rentalId = rentalId,
        commandType = commandType.name,
        parameters = parameters?.mapValues { it.value.toString() }
    )
}
