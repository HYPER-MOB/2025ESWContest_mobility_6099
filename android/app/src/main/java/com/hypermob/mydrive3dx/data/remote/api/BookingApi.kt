package com.hypermob.mydrive3dx.data.remote.api

import com.hypermob.mydrive3dx.domain.model.Booking
import com.hypermob.mydrive3dx.domain.model.CreateBookingRequest
import com.hypermob.mydrive3dx.domain.model.UpdateBookingRequest
import retrofit2.http.*

/**
 * Booking API Interface
 * 예약 관련 API
 */
interface BookingApi {

    /**
     * POST /bookings
     * 예약 생성
     */
    @POST("bookings")
    suspend fun createBooking(
        @Body request: CreateBookingRequest
    ): Booking

    /**
     * GET /bookings
     * 예약 목록 조회
     */
    @GET("bookings")
    suspend fun getBookings(
        @Query("user_id") userId: String
    ): List<Booking>

    /**
     * GET /bookings/{booking_id}
     * 특정 예약 조회
     */
    @GET("bookings/{booking_id}")
    suspend fun getBooking(
        @Path("booking_id") bookingId: String
    ): Booking

    /**
     * PATCH /bookings/{booking_id}
     * 예약 취소/완료
     */
    @PATCH("bookings/{booking_id}")
    suspend fun updateBooking(
        @Path("booking_id") bookingId: String,
        @Body request: UpdateBookingRequest
    ): Booking
}
