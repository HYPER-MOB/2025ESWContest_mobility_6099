package com.hypermob.mydrive3dx.domain.repository

import com.hypermob.mydrive3dx.domain.model.*
import kotlinx.coroutines.flow.Flow

/**
 * Rental Repository Interface
 *
 * 대여 관련 Repository 인터페이스 (Domain Layer)
 */
interface RentalRepository {

    /**
     * 현재 진행 중인 대여 목록 조회
     */
    suspend fun getActiveRentals(): Flow<Result<List<Rental>>>

    /**
     * 전체 대여 목록 조회
     */
    suspend fun getAllRentals(
        status: RentalStatus? = null,
        page: Int = 1,
        pageSize: Int = 20
    ): Flow<Result<List<Rental>>>

    /**
     * 대여 상세 정보 조회
     */
    suspend fun getRentalById(rentalId: String): Flow<Result<Rental>>

    /**
     * 대여 요청
     */
    suspend fun createRental(request: RentalRequest): Flow<Result<Rental>>

    /**
     * 대여 취소
     */
    suspend fun cancelRental(rentalId: String): Flow<Result<Unit>>

    /**
     * 차량 인수 (Picked 상태로 변경)
     */
    suspend fun pickupVehicle(rentalId: String): Flow<Result<Rental>>

    /**
     * 차량 반납 (Returned 상태로 변경)
     */
    suspend fun returnVehicle(rentalId: String): Flow<Result<Rental>>

    /**
     * 사용 가능한 차량 목록 조회
     */
    suspend fun getAvailableVehicles(): Flow<Result<List<Vehicle>>>
}
