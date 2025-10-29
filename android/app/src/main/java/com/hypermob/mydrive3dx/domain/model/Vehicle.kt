package com.hypermob.mydrive3dx.domain.model

/**
 * Vehicle Domain Model
 *
 * 차량 정보
 */
data class Vehicle(
    val vehicleId: String,
    val model: String,
    val manufacturer: String,
    val year: Int,
    val color: String,
    val licensePlate: String,
    val vehicleType: VehicleType,
    val fuelType: FuelType,
    val transmission: TransmissionType,
    val seatingCapacity: Int,
    val pricePerHour: Double,
    val pricePerDay: Double,
    val imageUrl: String?,
    val location: String,
    val latitude: Double?,
    val longitude: Double?,
    val status: VehicleStatus,
    val features: List<String> = emptyList(),
    val distance: Float? = null  // 사용자로부터의 거리 (km)
)
