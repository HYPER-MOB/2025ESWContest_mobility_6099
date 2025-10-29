package com.hypermob.mydrive3dx.domain.model

/**
 * Rental Request Domain Model
 *
 * 대여 요청
 */
data class RentalRequest(
    val vehicleId: String,
    val startTime: String,
    val endTime: String,
    val pickupLocation: String,
    val returnLocation: String
)
