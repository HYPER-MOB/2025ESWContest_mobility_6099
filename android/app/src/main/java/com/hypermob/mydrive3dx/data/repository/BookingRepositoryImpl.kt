package com.hypermob.mydrive3dx.data.repository

import com.hypermob.mydrive3dx.data.remote.api.BookingApi
import com.hypermob.mydrive3dx.domain.model.Booking
import com.hypermob.mydrive3dx.domain.model.CreateBookingRequest
import com.hypermob.mydrive3dx.domain.model.UpdateBookingRequest
import com.hypermob.mydrive3dx.domain.repository.BookingRepository
import javax.inject.Inject

/**
 * Booking Repository Implementation
 * 예약 관련 데이터 저장소 구현
 */
class BookingRepositoryImpl @Inject constructor(
    private val api: BookingApi
) : BookingRepository {

    override suspend fun createBooking(request: CreateBookingRequest): Booking {
        return api.createBooking(request)
    }

    override suspend fun getBookings(userId: String): List<Booking> {
        return api.getBookings(userId)
    }

    override suspend fun getBooking(bookingId: String): Booking {
        return api.getBooking(bookingId)
    }

    override suspend fun updateBooking(bookingId: String, status: String): Booking {
        return api.updateBooking(bookingId, UpdateBookingRequest(status))
    }

    override suspend fun cancelBooking(bookingId: String): Booking {
        return updateBooking(bookingId, "cancelled")
    }

    override suspend fun completeBooking(bookingId: String): Booking {
        return updateBooking(bookingId, "completed")
    }
}
