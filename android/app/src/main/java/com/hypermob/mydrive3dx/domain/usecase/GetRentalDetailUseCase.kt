package com.hypermob.mydrive3dx.domain.usecase

import com.hypermob.mydrive3dx.domain.model.Rental
import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.repository.RentalRepository
import kotlinx.coroutines.flow.Flow
import javax.inject.Inject

/**
 * Get Rental Detail UseCase
 *
 * 대여 상세 정보 조회
 */
class GetRentalDetailUseCase @Inject constructor(
    private val rentalRepository: RentalRepository
) {
    suspend operator fun invoke(rentalId: String): Flow<Result<Rental>> {
        return rentalRepository.getRentalById(rentalId)
    }
}
