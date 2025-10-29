package com.hypermob.mydrive3dx.domain.repository

import com.hypermob.mydrive3dx.domain.model.Booking
import com.hypermob.mydrive3dx.domain.model.CreateBookingRequest

/**
 * Booking Repository Interface
 * 예약 관련 데이터 저장소 인터페이스
 */
interface BookingRepository {
    suspend fun createBooking(request: CreateBookingRequest): Booking
    suspend fun getBookings(userId: String): List<Booking>
    suspend fun getBooking(bookingId: String): Booking
    suspend fun updateBooking(bookingId: String, status: String): Booking
    suspend fun cancelBooking(bookingId: String): Booking
    suspend fun completeBooking(bookingId: String): Booking
}
