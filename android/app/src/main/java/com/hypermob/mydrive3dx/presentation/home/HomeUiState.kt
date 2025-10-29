package com.hypermob.mydrive3dx.presentation.home

import com.hypermob.mydrive3dx.domain.model.Rental
import com.hypermob.mydrive3dx.domain.model.User

/**
 * Home UI State
 *
 * 홈 화면의 상태
 */
data class HomeUiState(
    val user: User? = null,
    val activeRentals: List<Rental> = emptyList(),
    val isLoading: Boolean = false,
    val errorMessage: String? = null
)
