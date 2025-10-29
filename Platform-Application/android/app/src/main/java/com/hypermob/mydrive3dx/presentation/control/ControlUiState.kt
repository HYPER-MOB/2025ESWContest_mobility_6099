package com.hypermob.mydrive3dx.presentation.control

import com.hypermob.mydrive3dx.domain.model.VehicleControl

/**
 * Control UI State
 *
 * 차량 제어 화면의 상태
 */
data class ControlUiState(
    val vehicleControl: VehicleControl? = null,
    val isLoading: Boolean = false,
    val isExecutingCommand: Boolean = false,
    val commandMessage: String? = null,
    val errorMessage: String? = null
)
