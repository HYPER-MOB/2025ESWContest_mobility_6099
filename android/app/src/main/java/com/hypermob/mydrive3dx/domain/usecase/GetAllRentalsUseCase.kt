package com.hypermob.mydrive3dx.domain.usecase

import com.hypermob.mydrive3dx.domain.model.Rental
import com.hypermob.mydrive3dx.domain.model.RentalStatus
import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.repository.RentalRepository
import kotlinx.coroutines.flow.Flow
import javax.inject.Inject

/**
 * Get All Rentals UseCase
 *
 * 전체 대여 목록 조회 (필터링 가능)
 */
class GetAllRentalsUseCase @Inject constructor(
    private val rentalRepository: RentalRepository
) {
    suspend operator fun invoke(
        status: RentalStatus? = null,
        page: Int = 1,
        pageSize: Int = 20
    ): Flow<Result<List<Rental>>> {
        return rentalRepository.getAllRentals(status, page, pageSize)
    }
}
