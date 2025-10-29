package com.hypermob.mydrive3dx.data.remote.dto

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

/**
 * Vehicle DTO
 * Server response uses snake_case
 */
@Serializable
data class VehicleDto(
    @SerialName("car_id")
    val vehicleId: String,
    val model: String,
    @SerialName("plate_number")
    val licensePlate: String = "",
    val year: Int = 2024,
    val color: String = "",
    @SerialName("image_url")
    val imageUrl: String? = null,
    @SerialName("location_address")
    val location: String = "",
    val status: String,
    @SerialName("price_per_hour")
    val pricePerHour: Double = 0.0,
    @SerialName("price_per_day")
    val pricePerDay: Double? = null,
    val manufacturer: String? = null,
    @SerialName("fuel_type")
    val fuelType: String? = null,
    val transmission: String? = null,
    @SerialName("seating_capacity")
    val seatingCapacity: Int? = null,
    val features: List<String>? = null,
    @SerialName("location_lat")
    val latitude: Double? = null,
    @SerialName("location_lng")
    val longitude: Double? = null
)

/**
 * Vehicles Response Wrapper
 * Matches server response: { status: 'success', data: { vehicles: [...], count: n } }
 */
@Serializable
data class VehiclesResponseData(
    val vehicles: List<VehicleDto>,
    val count: Int
)
