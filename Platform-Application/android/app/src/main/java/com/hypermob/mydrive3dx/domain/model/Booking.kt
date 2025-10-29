package com.hypermob.mydrive3dx.domain.model

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

/**
 * Booking Model
 * 예약 정보 (Rental과 유사하지만 API 응답에 맞춘 모델)
 */
@Serializable
data class Booking(
    @SerialName("booking_id")
    val bookingId: String,
    @SerialName("user_id")
    val userId: String,
    @SerialName("car_id")
    val carId: String,
    @SerialName("start_time")
    val startTime: String,  // ISO 8601 format
    @SerialName("end_time")
    val endTime: String,    // ISO 8601 format
    val status: BookingStatus,
    @SerialName("created_at")
    val createdAt: String? = null
)

@Serializable
enum class BookingStatus {
    pending,
    confirmed,
    active,
    completed,
    cancelled
}

/**
 * Booking Request
 * 예약 생성 요청
 */
@Serializable
data class CreateBookingRequest(
    @SerialName("user_id")
    val userId: String,
    @SerialName("car_id")
    val carId: String,
    @SerialName("start_time")
    val startTime: String,
    @SerialName("end_time")
    val endTime: String
)

/**
 * Booking Update Request
 * 예약 상태 업데이트
 */
@Serializable
data class UpdateBookingRequest(
    val status: String  // "cancelled" or "completed"
)
