package com.hypermob.mydrive3dx.presentation.home

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.usecase.GetActiveRentalsUseCase
import com.hypermob.mydrive3dx.domain.usecase.GetCurrentUserUseCase
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import javax.inject.Inject

/**
 * Home ViewModel
 *
 * 홈 화면의 비즈니스 로직 처리
 */
@HiltViewModel
class HomeViewModel @Inject constructor(
    private val getCurrentUserUseCase: GetCurrentUserUseCase,
    private val getActiveRentalsUseCase: GetActiveRentalsUseCase
) : ViewModel() {

    private val _uiState = MutableStateFlow(HomeUiState())
    val uiState: StateFlow<HomeUiState> = _uiState.asStateFlow()

    init {
        loadData()
    }

    /**
     * 데이터 로드 (사용자 정보 + 현재 대여)
     */
    fun loadData() {
        loadUserInfo()
        loadActiveRentals()
    }

    /**
     * 사용자 정보 로드
     */
    private fun loadUserInfo() {
        viewModelScope.launch {
            getCurrentUserUseCase().collect { result ->
                when (result) {
                    is Result.Success -> {
                        _uiState.update { it.copy(user = result.data) }
                    }
                    is Result.Error -> {
                        _uiState.update {
                            it.copy(errorMessage = result.exception.message)
                        }
                    }
                    is Result.Loading -> {
                        // 로딩 상태는 activeRentals와 함께 처리
                    }
                }
            }
        }
    }

    /**
     * 현재 진행 중인 대여 목록 로드
     */
    private fun loadActiveRentals() {
        viewModelScope.launch {
            getActiveRentalsUseCase().collect { result ->
                when (result) {
                    is Result.Loading -> {
                        _uiState.update { it.copy(isLoading = true) }
                    }
                    is Result.Success -> {
                        _uiState.update {
                            it.copy(
                                activeRentals = result.data,
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
