package com.hypermob.mydrive3dx.presentation.rental

import android.annotation.SuppressLint
import android.location.Location
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.hypermob.mydrive3dx.domain.model.Rental
import com.hypermob.mydrive3dx.domain.model.RentalStatus
import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.usecase.GetAllRentalsUseCase
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import javax.inject.Inject

/**
 * Rental List ViewModel
 *
 * 대여 목록 화면의 비즈니스 로직 처리
 */
@HiltViewModel
class RentalListViewModel @Inject constructor(
    private val getAllRentalsUseCase: GetAllRentalsUseCase
) : ViewModel() {

    private val _uiState = MutableStateFlow(RentalListUiState())
    val uiState: StateFlow<RentalListUiState> = _uiState.asStateFlow()

    private var userLocation: Location? = null

    init {
        loadRentals()
    }

    /**
     * 대여 목록 로드
     */
    fun loadRentals(status: RentalStatus? = _uiState.value.selectedFilter) {
        viewModelScope.launch {
            getAllRentalsUseCase(status = status).collect { result ->
                when (result) {
                    is Result.Loading -> {
                        _uiState.update {
                            it.copy(isLoading = true, isRefreshing = false)
                        }
                    }
                    is Result.Success -> {
                        _uiState.update {
                            it.copy(
                                rentals = result.data,
                                selectedFilter = status,
                                isLoading = false,
                                isRefreshing = false,
                                errorMessage = null
                            )
                        }
                    }
                    is Result.Error -> {
                        _uiState.update {
                            it.copy(
                                isLoading = false,
                                isRefreshing = false,
                                errorMessage = result.exception.message
                            )
                        }
                    }
                }
            }
        }
    }

    /**
     * Pull-to-Refresh
     */
    fun refresh() {
        viewModelScope.launch {
            _uiState.update { it.copy(isRefreshing = true) }
            loadRentals()
        }
    }

    /**
     * 필터 변경
     */
    fun setFilter(status: RentalStatus?) {
        loadRentals(status)
    }

    /**
     * 에러 메시지 초기화
     */
    fun clearError() {
        _uiState.update { it.copy(errorMessage = null) }
    }

    /**
     * 위치 기반 정렬 활성화 (시나리오2)
     */
    fun enableLocationSorting(latitude: Double, longitude: Double) {
        userLocation = Location("").apply {
            this.latitude = latitude
            this.longitude = longitude
        }
        sortRentalsByDistance(userLocation!!)
    }

    /**
     * 사용자 위치 업데이트 및 거리 계산 (간단한 버전)
     */
    private fun updateUserLocationSimple() {
        // 서울 시청 좌표를 기본값으로 사용 (시뮬레이션)
        userLocation = Location("").apply {
            latitude = 37.5666805
            longitude = 126.9784147
        }
        userLocation?.let { sortRentalsByDistance(it) }
    }

    /**
     * 대여 목록을 거리순으로 정렬
     */
    private fun sortRentalsByDistance(userLocation: Location) {
        _uiState.update { state ->
            val sortedRentals = state.rentals.map { rental ->
                // 픽업 위치 좌표가 있으면 거리 계산
                val distance = if (rental.pickupLatitude != null && rental.pickupLongitude != null) {
                    val rentalLocation = Location("").apply {
                        latitude = rental.pickupLatitude
                        longitude = rental.pickupLongitude
                    }
                    userLocation.distanceTo(rentalLocation) / 1000f // km로 변환
                } else {
                    // 더미 데이터를 위한 랜덤 거리 생성
                    (Math.random() * 50).toFloat() // 0~50km 랜덤
                }

                rental.copy(distance = distance)
            }.sortedWith(
                compareBy(
                    // 1. 대여 요청 상태인 것들을 먼저 (대여 가능)
                    { it.status == RentalStatus.PICKED || it.status == RentalStatus.RETURNED },
                    // 2. 거리가 가까운 순
                    { it.distance ?: Float.MAX_VALUE }
                )
            )

            state.copy(
                rentals = sortedRentals,
                isSortedByDistance = true
            )
        }
    }

    /**
     * 정렬 옵션 변경
     */
    fun setSortOption(option: SortOption) {
        when (option) {
            SortOption.DISTANCE -> {
                userLocation?.let { sortRentalsByDistance(it) }
                    ?: updateUserLocationSimple()
            }
            SortOption.PRICE -> {
                sortRentalsByPrice()
            }
            SortOption.DEFAULT -> {
                loadRentals()
            }
        }
    }

    /**
     * 가격순 정렬
     */
    private fun sortRentalsByPrice() {
        _uiState.update { state ->
            state.copy(
                rentals = state.rentals.sortedBy { it.totalPrice }
            )
        }
    }
}

/**
 * 정렬 옵션
 */
enum class SortOption {
    DEFAULT,
    DISTANCE,
    PRICE
}
