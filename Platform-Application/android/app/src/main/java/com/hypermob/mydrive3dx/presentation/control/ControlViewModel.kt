package com.hypermob.mydrive3dx.presentation.control

import androidx.lifecycle.SavedStateHandle
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.usecase.ExecuteVehicleCommandUseCase
import com.hypermob.mydrive3dx.domain.usecase.GetActiveRentalsUseCase
import com.hypermob.mydrive3dx.domain.usecase.GetVehicleControlUseCase
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import javax.inject.Inject

/**
 * Control ViewModel
 *
 * 차량 제어 화면의 비즈니스 로직 처리
 */
@HiltViewModel
class ControlViewModel @Inject constructor(
    private val getActiveRentalsUseCase: GetActiveRentalsUseCase,
    private val getVehicleControlUseCase: GetVehicleControlUseCase,
    private val executeVehicleCommandUseCase: ExecuteVehicleCommandUseCase,
    savedStateHandle: SavedStateHandle
) : ViewModel() {

    private val _uiState = MutableStateFlow(ControlUiState())
    val uiState: StateFlow<ControlUiState> = _uiState.asStateFlow()

    private var currentRentalId: String? = null

    init {
        loadActiveRental()
    }

    /**
     * 현재 진행 중인 대여 조회
     */
    private fun loadActiveRental() {
        viewModelScope.launch {
            getActiveRentalsUseCase().collect { result ->
                when (result) {
                    is Result.Loading -> {
                        _uiState.update { it.copy(isLoading = true) }
                    }
                    is Result.Success -> {
                        val activeRental = result.data.firstOrNull()
                        if (activeRental != null) {
                            currentRentalId = activeRental.rentalId
                            loadVehicleControl(activeRental.rentalId)
                        } else {
                            _uiState.update {
                                it.copy(
                                    isLoading = false,
                                    errorMessage = "진행 중인 대여가 없습니다"
                                )
                            }
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

    /**
     * 차량 제어 상태 조회
     */
    private fun loadVehicleControl(rentalId: String) {
        viewModelScope.launch {
            getVehicleControlUseCase(rentalId).collect { result ->
                when (result) {
                    is Result.Loading -> {
                        _uiState.update { it.copy(isLoading = true) }
                    }
                    is Result.Success -> {
                        _uiState.update {
                            it.copy(
                                vehicleControl = result.data,
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

    /**
     * 문 잠금
     */
    fun lockDoors() {
        currentRentalId?.let { rentalId ->
            executeCommand { executeVehicleCommandUseCase.lockDoors(rentalId) }
        }
    }

    /**
     * 문 잠금 해제
     */
    fun unlockDoors() {
        currentRentalId?.let { rentalId ->
            executeCommand { executeVehicleCommandUseCase.unlockDoors(rentalId) }
        }
    }

    /**
     * 시동 켜기
     */
    fun startEngine() {
        currentRentalId?.let { rentalId ->
            executeCommand { executeVehicleCommandUseCase.startEngine(rentalId) }
        }
    }

    /**
     * 시동 끄기
     */
    fun stopEngine() {
        currentRentalId?.let { rentalId ->
            executeCommand { executeVehicleCommandUseCase.stopEngine(rentalId) }
        }
    }

    /**
     * 에어컨 켜기
     */
    fun turnOnClimate(temperature: Double = 22.0) {
        currentRentalId?.let { rentalId ->
            executeCommand { executeVehicleCommandUseCase.turnOnClimate(rentalId, temperature) }
        }
    }

    /**
     * 에어컨 끄기
     */
    fun turnOffClimate() {
        currentRentalId?.let { rentalId ->
            executeCommand { executeVehicleCommandUseCase.turnOffClimate(rentalId) }
        }
    }

    /**
     * 경적 울리기
     */
    fun honkHorn() {
        currentRentalId?.let { rentalId ->
            executeCommand { executeVehicleCommandUseCase.honkHorn(rentalId) }
        }
    }

    /**
     * 라이트 깜빡이기
     */
    fun flashLights() {
        currentRentalId?.let { rentalId ->
            executeCommand { executeVehicleCommandUseCase.flashLights(rentalId) }
        }
    }

    /**
     * 명령 실행 공통 로직
     */
    private fun executeCommand(command: suspend () -> kotlinx.coroutines.flow.Flow<Result<com.hypermob.mydrive3dx.domain.model.ControlCommandResponse>>) {
        viewModelScope.launch {
            command().collect { result ->
                when (result) {
                    is Result.Loading -> {
                        _uiState.update { it.copy(isExecutingCommand = true, commandMessage = null) }
                    }
                    is Result.Success -> {
                        _uiState.update {
                            it.copy(
                                isExecutingCommand = false,
                                commandMessage = result.data.message ?: "명령이 실행되었습니다",
                                vehicleControl = result.data.updatedControl ?: it.vehicleControl
                            )
                        }
                    }
                    is Result.Error -> {
                        _uiState.update {
                            it.copy(
                                isExecutingCommand = false,
                                errorMessage = result.exception.message
                            )
                        }
                    }
                }
            }
        }
    }

    /**
     * 에러 메시지 초기화
     */
    fun clearError() {
        _uiState.update { it.copy(errorMessage = null, commandMessage = null) }
    }

    /**
     * 새로고침
     */
    fun refresh() {
        currentRentalId?.let { loadVehicleControl(it) }
    }
}
