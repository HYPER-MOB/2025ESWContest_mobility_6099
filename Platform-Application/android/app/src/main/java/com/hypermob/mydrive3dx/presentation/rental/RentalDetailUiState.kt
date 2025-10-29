package com.hypermob.mydrive3dx.presentation.rental

import com.hypermob.mydrive3dx.domain.model.Rental

/**
 * Rental Detail UI State
 *
 * 대여 상세 화면의 상태
 */
data class RentalDetailUiState(
    val rental: Rental? = null,
    val isLoading: Boolean = false,
    val errorMessage: String? = null
)
