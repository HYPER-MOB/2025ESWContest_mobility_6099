package com.hypermob.mydrive3dx.presentation.mfa

import androidx.lifecycle.SavedStateHandle
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.hypermob.mydrive3dx.data.remote.api.MfaApi
import com.hypermob.mydrive3dx.domain.usecase.GetCurrentUserUseCase
import com.hypermob.mydrive3dx.domain.model.Result
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import javax.inject.Inject

/**
 * MFA ViewModel
 *
 * MFA 인증 화면의 비즈니스 로직 처리 (간단 버전)
 */
@HiltViewModel
class MfaViewModel @Inject constructor(
    savedStateHandle: SavedStateHandle,
    private val mfaApi: MfaApi,
    private val getCurrentUserUseCase: GetCurrentUserUseCase
) : ViewModel() {

    private val _uiState = MutableStateFlow(MfaUiState())
    val uiState: StateFlow<MfaUiState> = _uiState.asStateFlow()

    private val rentalId: String? = savedStateHandle.get<String>("rentalId")
    private var pollingJob: Job? = null
    private var userId: String? = null

    init {
        _uiState.update { it.copy(rentalId = rentalId) }
        loadUserInfo()
        startPolling()
    }

    /**
     * 사용자 정보 로드
     */
    private fun loadUserInfo() {
        viewModelScope.launch {
            getCurrentUserUseCase().collect { result ->
                if (result is Result.Success) {
                    userId = result.data.userId
                }
            }
        }
    }

    /**
     * MFA 인증 상태 Polling 시작 (시나리오3)
     */
    fun startPolling() {
        pollingJob?.cancel()
        pollingJob = viewModelScope.launch {
            while (isActive && !_uiState.value.isComplete) {
                pollAuthenticationStatus()
                delay(2000) // 2초마다 폴링
            }
        }
    }

    /**
     * 인증 상태 조회
     */
    private suspend fun pollAuthenticationStatus() {
        userId?.let { uid ->
            try {
                val response = mfaApi.getAuthResult(uid, rentalId)
                _uiState.update {
                    it.copy(
                        faceVerified = response.face_verified,
                        nfcVerified = response.nfc_verified,
                        bleVerified = response.ble_verified,
                        isComplete = response.all_verified
                    )
                }

                if (response.all_verified) {
                    pollingJob?.cancel()
                    // 3초 후 자동 전환
                    delay(3000)
                    _uiState.update { it.copy(shouldNavigateToControl = true) }
                }
            } catch (e: Exception) {
                // 에러 무시하고 계속 폴링
            }
        }
    }

    /**
     * Face Recognition 시작 (시뮬레이션)
     */
    fun startFaceVerification() {
        viewModelScope.launch {
            _uiState.update { it.copy(isLoading = true) }
            delay(2000) // 시뮬레이션
            _uiState.update {
                it.copy(
                    faceVerified = true,
                    isLoading = false
                )
            }
        }
    }

    /**
     * Face Recognition 완료 설정 (실제 카메라 감지 후 호출)
     */
    fun setFaceVerified() {
        _uiState.update {
            it.copy(faceVerified = true, isLoading = false)
        }
    }

    /**
     * NFC 인증 시작 (시뮬레이션)
     */
    fun startNfcVerification() {
        viewModelScope.launch {
            _uiState.update { it.copy(isLoading = true) }
            delay(1500) // 시뮬레이션
            _uiState.update {
                it.copy(
                    nfcVerified = true,
                    isLoading = false
                )
            }
        }
    }

    /**
     * NFC 인증 완료 설정 (실제 NFC 태그 감지 후 호출)
     */
    fun setNfcVerified() {
        _uiState.update {
            it.copy(nfcVerified = true, isLoading = false)
        }
    }

    /**
     * BLE 인증 시작 (시뮬레이션)
     */
    fun startBleVerification() {
        viewModelScope.launch {
            _uiState.update { it.copy(isLoading = true) }
            delay(1500) // 시뮬레이션
            _uiState.update {
                it.copy(
                    bleVerified = true,
                    isLoading = false
                )
            }
            checkCompletion()
        }
    }

    /**
     * BLE 인증 완료 설정 (실제 BLE 연결 후 호출)
     */
    fun setBleVerified() {
        _uiState.update {
            it.copy(bleVerified = true, isLoading = false)
        }
        checkCompletion()
    }

    /**
     * 모든 인증 완료 확인
     */
    private fun checkCompletion() {
        if (_uiState.value.allVerified) {
            _uiState.update { it.copy(isComplete = true) }
        }
    }

    /**
     * Polling 중지
     */
    fun stopPolling() {
        pollingJob?.cancel()
        pollingJob = null
    }

    override fun onCleared() {
        super.onCleared()
        stopPolling()
    }
}
