package com.hypermob.mydrive3dx.presentation.vehicle

import android.location.Location
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.hypermob.mydrive3dx.domain.model.Vehicle
import com.hypermob.mydrive3dx.domain.model.VehicleStatus
import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.usecase.GetAvailableVehiclesUseCase
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import javax.inject.Inject

/**
 * Vehicle List ViewModel
 * 시나리오2: 대여 가능한 차량 목록 관리
 */
@HiltViewModel
class VehicleListViewModel @Inject constructor(
    private val getAvailableVehiclesUseCase: GetAvailableVehiclesUseCase
) : ViewModel() {

    private val _uiState = MutableStateFlow(VehicleListUiState())
    val uiState: StateFlow<VehicleListUiState> = _uiState.asStateFlow()

    // 사용자 위치 (시뮬레이션)
    private val userLocation = Location("").apply {
        latitude = 37.5666805  // 서울 시청
        longitude = 126.9784147
    }

    init {
        loadVehicles()
    }

    fun loadVehicles() {
        viewModelScope.launch {
            getAvailableVehiclesUseCase().collect { result ->
                when (result) {
                    is Result.Loading -> {
                        _uiState.update { it.copy(isLoading = true) }
                    }
                    is Result.Success -> {
                        val vehicles = result.data
                        // 거리 계산 및 정렬
                        val vehiclesWithDistance = calculateDistances(vehicles)
                        val sortedVehicles = sortVehicles(vehiclesWithDistance, _uiState.value.sortByDistance)

                        _uiState.update {
                            it.copy(
                                vehicles = sortedVehicles,
                                isLoading = false,
                                errorMessage = null
                            )
                        }
                    }
                    is Result.Error -> {
                        _uiState.update {
                            it.copy(
                                isLoading = false,
                                errorMessage = result.exception.message
                            )
                        }
                    }
                }
            }
        }
    }

    fun toggleSortOption() {
        val newSortByDistance = !_uiState.value.sortByDistance
        _uiState.update {
            it.copy(
                sortByDistance = newSortByDistance,
                vehicles = sortVehicles(it.vehicles, newSortByDistance)
            )
        }
    }

    private fun calculateDistances(vehicles: List<Vehicle>): List<Vehicle> {
        return vehicles.map { vehicle ->
            val distance = if (vehicle.latitude != null && vehicle.longitude != null) {
                val vehicleLocation = Location("").apply {
                    latitude = vehicle.latitude
                    longitude = vehicle.longitude
                }
                userLocation.distanceTo(vehicleLocation) / 1000f // km로 변환
            } else {
                // 위치 정보가 없으면 랜덤 거리 (시뮬레이션)
                (Math.random() * 20).toFloat()
            }
            vehicle.copy(distance = distance)
        }
    }

    private fun sortVehicles(vehicles: List<Vehicle>, sortByDistance: Boolean): List<Vehicle> {
        return if (sortByDistance) {
            // 시나리오2: 대여 가능한 차량이 상단에, 거리순으로 정렬
            vehicles.sortedWith(
                compareBy(
                    { it.status != VehicleStatus.AVAILABLE },  // 대여 가능한 차량 우선
                    { it.distance ?: Float.MAX_VALUE }         // 가까운 순
                )
            )
        } else {
            // 가격순 정렬
            vehicles.sortedWith(
                compareBy(
                    { it.status != VehicleStatus.AVAILABLE },  // 대여 가능한 차량 우선
                    { it.pricePerHour }                        // 저렴한 순
                )
            )
        }
    }

    fun clearError() {
        _uiState.update { it.copy(errorMessage = null) }
    }
}

data class VehicleListUiState(
    val vehicles: List<Vehicle> = emptyList(),
    val isLoading: Boolean = false,
    val errorMessage: String? = null,
    val sortByDistance: Boolean = true  // 기본값: 거리순 정렬
)

