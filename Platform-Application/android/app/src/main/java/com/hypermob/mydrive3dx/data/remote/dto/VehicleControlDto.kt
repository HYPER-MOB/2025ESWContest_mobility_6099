package com.hypermob.mydrive3dx.data.remote.dto

import kotlinx.serialization.Serializable

/**
 * Vehicle Control DTO
 */
@Serializable
data class VehicleControlDto(
    val vehicleId: String,
    val rentalId: String,
    val status: VehicleControlStatusDto,
    val doors: DoorStatusDto,
    val engine: EngineStatusDto,
    val climate: ClimateStatusDto,
    val location: VehicleLocationDto? = null,
    val lastUpdated: String
)

@Serializable
data class VehicleControlStatusDto(
    val isOnline: Boolean,
    val batteryLevel: Int,
    val fuelLevel: Int,
    val odometer: Double
)

@Serializable
data class DoorStatusDto(
    val frontLeft: Boolean,
    val frontRight: Boolean,
    val rearLeft: Boolean,
    val rearRight: Boolean,
    val trunk: Boolean
)

@Serializable
data class EngineStatusDto(
    val isRunning: Boolean,
    val temperature: Double,
    val rpm: Int
)

@Serializable
data class ClimateStatusDto(
    val isOn: Boolean,
    val temperature: Double,
    val fanSpeed: Int
)

@Serializable
data class VehicleLocationDto(
    val latitude: Double,
    val longitude: Double,
    val address: String? = null
)

/**
 * Control Command Request DTO
 */
@Serializable
data class ControlCommandRequestDto(
    val rentalId: String,
    val commandType: String,
    val parameters: Map<String, String>? = null
)

/**
 * Control Command Response DTO
 */
@Serializable
data class ControlCommandResponseDto(
    val success: Boolean,
    val message: String? = null,
    val updatedControl: VehicleControlDto? = null
)
