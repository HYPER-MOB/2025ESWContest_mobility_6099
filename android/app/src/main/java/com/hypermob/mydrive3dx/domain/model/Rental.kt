package com.hypermob.mydrive3dx.domain.model

/**
 * Rental Domain Model
 *
 * 대여 정보
 */
data class Rental(
    val rentalId: String,
    val vehicleId: String,
    val userId: String,
    val vehicle: Vehicle?,
    val startTime: String,
    val endTime: String,
    val status: RentalStatus,
    val totalPrice: Double,
    val pickupLocation: String,
    val returnLocation: String,
    val createdAt: String,
    val updatedAt: String,

    // MFA 인증 상태
    val mfaStatus: MfaStatus? = null,

    // 위치 정보 (시나리오2 - 거리순 정렬용)
    val pickupLatitude: Double? = null,
    val pickupLongitude: Double? = null,
    val distance: Float? = null  // 사용자 현재 위치로부터의 거리 (km)
)

/**
 * MFA Status
 */
data class MfaStatus(
    val faceVerified: Boolean,
    val nfcVerified: Boolean,
    val bleVerified: Boolean,
    val allPassed: Boolean
)
