package com.hypermob.mydrive3dx.data.remote.dto

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

/**
 * Rental DTO (maps to Booking on server)
 * Server response uses snake_case
 */
@Serializable
data class RentalDto(
    @SerialName("booking_id")
    val rentalId: String,
    @SerialName("car_id")
    val vehicleId: String,
    @SerialName("user_id")
    val userId: String,
    val vehicle: VehicleDto? = null,
    @SerialName("start_time")
    val startTime: String,
    @SerialName("end_time")
    val endTime: String,
    val status: String,
    @SerialName("total_price")
    val totalPrice: Double? = null,
    @SerialName("pickup_location")
    val pickupLocation: String? = null,
    @SerialName("return_location")
    val returnLocation: String? = null,
    @SerialName("created_at")
    val createdAt: String? = null,
    @SerialName("updated_at")
    val updatedAt: String? = null,
    @SerialName("mfa_status")
    val mfaStatus: MfaStatusDto? = null
)

/**
 * MFA Status DTO
 */
@Serializable
data class MfaStatusDto(
    @SerialName("face_verified")
    val faceVerified: Boolean,
    @SerialName("nfc_verified")
    val nfcVerified: Boolean,
    @SerialName("ble_verified")
    val bleVerified: Boolean,
    @SerialName("all_passed")
    val allPassed: Boolean
)

/**
 * Rental Request DTO (maps to Booking request on server)
 */
@Serializable
data class RentalRequestDto(
    @SerialName("user_id")
    val userId: String,
    @SerialName("car_id")
    val vehicleId: String,
    @SerialName("start_time")
    val startTime: String,
    @SerialName("end_time")
    val endTime: String
)
