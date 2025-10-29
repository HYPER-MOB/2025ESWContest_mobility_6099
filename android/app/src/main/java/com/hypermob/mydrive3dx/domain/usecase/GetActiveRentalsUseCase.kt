package com.hypermob.mydrive3dx.domain.usecase

import com.hypermob.mydrive3dx.domain.model.Rental
import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.repository.RentalRepository
import kotlinx.coroutines.flow.Flow
import javax.inject.Inject

/**
 * Get Active Rentals UseCase
 *
 * 현재 진행 중인 대여 목록 조회
 */
class GetActiveRentalsUseCase @Inject constructor(
    private val rentalRepository: RentalRepository
) {
    suspend operator fun invoke(): Flow<Result<List<Rental>>> {
        return rentalRepository.getActiveRentals()
    }
}
