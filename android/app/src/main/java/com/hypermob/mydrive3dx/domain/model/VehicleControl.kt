package com.hypermob.mydrive3dx.domain.model

/**
 * Vehicle Control Domain Model
 *
 * 차량 제어 상태
 */
data class VehicleControl(
    val vehicleId: String,
    val rentalId: String,
    val status: VehicleControlStatus,
    val doors: DoorStatus,
    val engine: EngineStatus,
    val climate: ClimateStatus,
    val location: VehicleLocation?,
    val lastUpdated: String
)

/**
 * Vehicle Control Status
 */
data class VehicleControlStatus(
    val isOnline: Boolean,
    val batteryLevel: Int, // 0-100
    val fuelLevel: Int, // 0-100
    val odometer: Double // km
)

/**
 * Door Status
 */
data class DoorStatus(
    val frontLeft: Boolean, // true = locked
    val frontRight: Boolean,
    val rearLeft: Boolean,
    val rearRight: Boolean,
    val trunk: Boolean
)

/**
 * Engine Status
 */
data class EngineStatus(
    val isRunning: Boolean,
    val temperature: Double, // Celsius
    val rpm: Int
)

/**
 * Climate Status
 */
data class ClimateStatus(
    val isOn: Boolean,
    val temperature: Double, // Celsius
    val fanSpeed: Int // 0-5
)

/**
 * Vehicle Location
 */
data class VehicleLocation(
    val latitude: Double,
    val longitude: Double,
    val address: String?
)
