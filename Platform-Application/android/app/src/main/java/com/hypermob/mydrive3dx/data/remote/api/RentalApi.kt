package com.hypermob.mydrive3dx.data.remote.api

import com.hypermob.mydrive3dx.data.remote.dto.ApiResponse
import com.hypermob.mydrive3dx.data.remote.dto.RentalDto
import com.hypermob.mydrive3dx.data.remote.dto.RentalRequestDto
import com.hypermob.mydrive3dx.data.remote.dto.VehiclesResponseData
import retrofit2.http.*

/**
 * Rental API Interface
 *
 * 대여 관련 API 엔드포인트
 * Server uses /bookings endpoint
 */
interface RentalApi {

    /**
     * 현재 진행 중인 대여 목록 조회
     * GET /bookings?status=active
     */
    @GET("bookings")
    suspend fun getActiveRentals(
        @Query("status") status: String = "active"
    ): ApiResponse<List<RentalDto>>

    /**
     * 전체 대여 목록 조회
     * GET /bookings?user_id={userId}&status={status}
     */
    @GET("bookings")
    suspend fun getAllRentals(
        @Query("user_id") userId: String? = null,
        @Query("status") status: String? = null,
        @Query("page") page: Int = 1,
        @Query("pageSize") pageSize: Int = 20
    ): ApiResponse<List<RentalDto>>

    /**
     * 대여 상세 정보 조회
     * GET /bookings/{booking_id}
     */
    @GET("bookings/{booking_id}")
    suspend fun getRentalById(
        @Path("booking_id") rentalId: String
    ): ApiResponse<RentalDto>

    /**
     * 대여 요청 (예약 생성)
     * POST /bookings
     */
    @POST("bookings")
    suspend fun createRental(
        @Body request: RentalRequestDto
    ): ApiResponse<RentalDto>

    /**
     * 대여 취소
     * PATCH /bookings/{booking_id}
     */
    @PATCH("bookings/{booking_id}")
    suspend fun cancelRental(
        @Path("booking_id") rentalId: String,
        @Body status: Map<String, String> = mapOf("status" to "cancelled")
    ): ApiResponse<RentalDto>

    /**
     * 차량 인수
     * PATCH /bookings/{booking_id}
     */
    @PATCH("bookings/{booking_id}")
    suspend fun pickupVehicle(
        @Path("booking_id") rentalId: String,
        @Body status: Map<String, String> = mapOf("status" to "active")
    ): ApiResponse<RentalDto>

    /**
     * 차량 반납
     * PATCH /bookings/{booking_id}
     */
    @PATCH("bookings/{booking_id}")
    suspend fun returnVehicle(
        @Path("booking_id") rentalId: String,
        @Body status: Map<String, String> = mapOf("status" to "completed")
    ): ApiResponse<RentalDto>

    /**
     * 사용 가능한 차량 목록 조회
     * GET /vehicles?status=available
     */
    @GET("vehicles")
    suspend fun getAvailableVehicles(
        @Query("status") status: String = "available"
    ): ApiResponse<VehiclesResponseData>
}
