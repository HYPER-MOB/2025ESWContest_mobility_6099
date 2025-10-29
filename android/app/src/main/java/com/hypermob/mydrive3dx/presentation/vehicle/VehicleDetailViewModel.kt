package com.hypermob.mydrive3dx.presentation.vehicle

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.hypermob.mydrive3dx.domain.model.Vehicle
import com.hypermob.mydrive3dx.domain.model.VehicleStatus
import com.hypermob.mydrive3dx.domain.model.VehicleType
import com.hypermob.mydrive3dx.domain.model.FuelType
import com.hypermob.mydrive3dx.domain.model.TransmissionType
import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.repository.RentalRepository
import com.hypermob.mydrive3dx.domain.repository.VehicleRepository
import com.hypermob.mydrive3dx.domain.usecase.GetCurrentUserUseCase
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import javax.inject.Inject

/**
 * Vehicle Detail ViewModel
 * 시나리오2: 차량 상세 정보 및 대여 처리
 */
@HiltViewModel
class VehicleDetailViewModel @Inject constructor(
    private val vehicleRepository: VehicleRepository,
    private val rentalRepository: RentalRepository,
    private val getCurrentUserUseCase: GetCurrentUserUseCase
) : ViewModel() {

    private val _uiState = MutableStateFlow(VehicleDetailUiState())
    val uiState: StateFlow<VehicleDetailUiState> = _uiState.asStateFlow()

    private var currentUserId: String? = null

    init {
        loadCurrentUser()
    }

    private fun loadCurrentUser() {
        viewModelScope.launch {
            getCurrentUserUseCase().collect { result ->
                when (result) {
                    is Result.Success -> {
                        currentUserId = result.data.userId
                    }
                    is Result.Error -> {
                        // 테스트용 임시 userId
                        currentUserId = "test_user_${System.currentTimeMillis()}"
                    }
                    is Result.Loading -> {}
                }
            }
        }
    }

    fun loadVehicle(vehicleId: String) {
        viewModelScope.launch {
            _uiState.update { it.copy(isLoading = true) }

            try {
                // TODO: 실제 API 호출
                // val vehicle = vehicleRepository.getVehicle(vehicleId)

                // 테스트용 더미 데이터
                val vehicle = when (vehicleId) {
                    "vehicle1" -> Vehicle(
                        vehicleId = "vehicle1",
                        model = "아이오닉 5",
                        manufacturer = "현대",
                        year = 2024,
                        color = "화이트",
                        licensePlate = "서울 12가 3456",
                        vehicleType = VehicleType.ELECTRIC,
                        fuelType = FuelType.ELECTRIC,
                        transmission = TransmissionType.AUTOMATIC,
                        seatingCapacity = 5,
                        pricePerHour = 15000.0,
                        pricePerDay = 120000.0,
                        imageUrl = null,
                        location = "강남역 주차장",
                        latitude = 37.497942,
                        longitude = 127.027621,
                        status = VehicleStatus.AVAILABLE,
                        features = listOf("자율주행", "무선충전", "빌트인캠", "열선시트", "통풍시트")
                    )
                    "vehicle2" -> Vehicle(
                        vehicleId = "vehicle2",
                        model = "Model 3",
                        manufacturer = "테슬라",
                        year = 2023,
                        color = "블랙",
                        licensePlate = "서울 34나 5678",
                        vehicleType = VehicleType.ELECTRIC,
                        fuelType = FuelType.ELECTRIC,
                        transmission = TransmissionType.AUTOMATIC,
                        seatingCapacity = 5,
                        pricePerHour = 20000.0,
                        pricePerDay = 150000.0,
                        imageUrl = null,
                        location = "판교역 주차장",
                        latitude = 37.394776,
                        longitude = 127.111047,
                        status = VehicleStatus.AVAILABLE,
                        features = listOf("오토파일럿", "센트리모드", "캠핑모드", "프리미엄 사운드")
                    )
                    else -> Vehicle(
                        vehicleId = vehicleId,
                        model = "GV70",
                        manufacturer = "제네시스",
                        year = 2024,
                        color = "실버",
                        licensePlate = "서울 56다 7890",
                        vehicleType = VehicleType.SUV,
                        fuelType = FuelType.GASOLINE,
                        transmission = TransmissionType.AUTOMATIC,
                        seatingCapacity = 5,
                        pricePerHour = 18000.0,
                        pricePerDay = 140000.0,
                        imageUrl = null,
                        location = "서울역 주차장",
                        latitude = 37.554722,
                        longitude = 126.970833,
                        status = VehicleStatus.AVAILABLE,
                        features = listOf("통풍시트", "마사지시트", "HUD", "360도 카메라")
                    )
                }

                _uiState.update {
                    it.copy(
                        vehicle = vehicle,
                        isLoading = false
                    )
                }
            } catch (e: Exception) {
                _uiState.update {
                    it.copy(
                        isLoading = false,
                        errorMessage = e.message
                    )
                }
            }
        }
    }

    fun requestRental(startTime: String, endTime: String) {
        val vehicle = _uiState.value.vehicle ?: return
        val userId = currentUserId ?: return

        viewModelScope.launch {
            _uiState.update { it.copy(rentalState = RentalState.REQUESTING) }

            try {
                // TODO: 실제 API 호출
                // val rentalId = rentalRepository.requestRental(
                //     vehicleId = vehicle.vehicleId,
                //     userId = userId,
                //     startTime = startTime,
                //     endTime = endTime,
                //     pickupLocation = vehicle.location
                // )

                // 시나리오2: 대여 요청 시뮬레이션
                kotlinx.coroutines.delay(1500)

                // 더미 rentalId 생성
                val rentalId = "rental_${System.currentTimeMillis()}"

                _uiState.update {
                    it.copy(
                        rentalState = RentalState.SUCCESS,
                        rentalId = rentalId
                    )
                }
            } catch (e: Exception) {
                _uiState.update {
                    it.copy(
                        rentalState = RentalState.ERROR,
                        errorMessage = e.message
                    )
                }
            }
        }
    }

    fun clearError() {
        _uiState.update { it.copy(errorMessage = null) }
    }
}

data class VehicleDetailUiState(
    val vehicle: Vehicle? = null,
    val isLoading: Boolean = false,
    val errorMessage: String? = null,
    val rentalState: RentalState = RentalState.IDLE,
    val rentalId: String? = null
)

enum class RentalState {
    IDLE,
    REQUESTING,
    SUCCESS,
    ERROR
}