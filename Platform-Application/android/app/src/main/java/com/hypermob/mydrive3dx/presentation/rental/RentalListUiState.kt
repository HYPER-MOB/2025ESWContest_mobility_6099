package com.hypermob.mydrive3dx.presentation.rental

import com.hypermob.mydrive3dx.domain.model.Rental
import com.hypermob.mydrive3dx.domain.model.RentalStatus

/**
 * Rental List UI State
 *
 * 대여 목록 화면의 상태
 */
data class RentalListUiState(
    val rentals: List<Rental> = emptyList(),
    val selectedFilter: RentalStatus? = null,
    val isLoading: Boolean = false,
    val isRefreshing: Boolean = false,
    val errorMessage: String? = null,
    // 시나리오2 - 거리순 정렬
    val isSortedByDistance: Boolean = false,
    val currentSortOption: SortOption = SortOption.DEFAULT
)
