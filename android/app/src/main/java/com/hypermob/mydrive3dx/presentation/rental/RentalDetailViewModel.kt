package com.hypermob.mydrive3dx.presentation.rental

import androidx.lifecycle.SavedStateHandle
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.usecase.GetRentalDetailUseCase
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import javax.inject.Inject

/**
 * Rental Detail ViewModel
 *
 * 대여 상세 화면의 비즈니스 로직 처리
 */
@HiltViewModel
class RentalDetailViewModel @Inject constructor(
    private val getRentalDetailUseCase: GetRentalDetailUseCase,
    savedStateHandle: SavedStateHandle
) : ViewModel() {

    private val _uiState = MutableStateFlow(RentalDetailUiState())
    val uiState: StateFlow<RentalDetailUiState> = _uiState.asStateFlow()

    private val rentalId: String? = savedStateHandle.get<String>("rentalId")

    init {
        rentalId?.let { loadRental(it) }
    }

    /**
     * 대여 상세 정보 로드
     */
    fun loadRental(rentalId: String) {
        viewModelScope.launch {
            getRentalDetailUseCase(rentalId).collect { result ->
                when (result) {
                    is Result.Loading -> {
                        _uiState.update { it.copy(isLoading = true) }
                    }
                    is Result.Success -> {
                        _uiState.update {
                            it.copy(
                                rental = result.data,
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
     * 에러 메시지 초기화
     */
    fun clearError() {
        _uiState.update { it.copy(errorMessage = null) }
    }
}
