package com.hypermob.mydrive3dx.presentation.mfa

import android.app.Activity
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.hypermob.mydrive3dx.data.hardware.BleManager
import com.hypermob.mydrive3dx.data.hardware.NfcManager
import com.hypermob.mydrive3dx.data.local.TokenManager
import com.hypermob.mydrive3dx.domain.model.MfaStepStatus
import com.hypermob.mydrive3dx.domain.usecase.AuthenticateMfaUseCase
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.*
import kotlinx.coroutines.launch
import javax.inject.Inject

/**
 * MFA Authentication State
 * MFA 인증 상태
 *
 * android_mvp 로직 기반:
 * - faceRegistered: 얼굴 등록 완료 여부
 * - bleConnected: BLE 연결 및 "ACCESS" 쓰기 완료 여부
 * - nfcTagRead: NFC 태그 UID 읽기 완료 여부
 */
data class MfaAuthState(
    val faceStatus: MfaStepStatus = MfaStepStatus.Pending,
    val bleStatus: MfaStepStatus = MfaStepStatus.Pending,
    val nfcStatus: MfaStepStatus = MfaStepStatus.Pending,
    val faceRegistered: Boolean = false,  // 얼굴 등록 완료
    val bleConnected: Boolean = false,    // BLE "ACCESS" 쓰기 완료
    val nfcTagUid: String? = null,        // NFC 태그에서 읽은 UID
    val isLoading: Boolean = false,
    val error: String? = null,
    val authSuccess: Boolean = false,
    val sessionId: String? = null
)

/**
 * MFA Authentication ViewModel
 * MFA 인증 뷰모델
 *
 * android_mvp 플로우:
 * 1. Face Registration (FaceRegisterScreen에서 완료) → onFaceRegistered 호출
 * 2. BLE Connection → scanBleDevice 호출
 * 3. NFC Tag Read → enableNfcReaderMode 호출
 * 4. Server Verification → verifyMfaWithServer 자동 호출
 */
@HiltViewModel
class MfaAuthViewModel @Inject constructor(
    private val bleManager: BleManager,
    private val nfcManager: NfcManager,
    private val tokenManager: TokenManager,
    private val authenticateMfaUseCase: AuthenticateMfaUseCase
) : ViewModel() {

    private val _state = MutableStateFlow(MfaAuthState())
    val state: StateFlow<MfaAuthState> = _state.asStateFlow()

    /**
     * 얼굴 등록 완료
     * FaceRegisterScreen에서 호출
     */
    fun onFaceRegistered(userId: String, faceId: String, nfcUid: String) {
        viewModelScope.launch {
            // TokenManager에 저장 (이미 AuthRepository에서 저장되지만 확인차)
            tokenManager.saveMfaInfo(userId, faceId, nfcUid)

            _state.update { it.copy(
                faceRegistered = true,
                faceStatus = MfaStepStatus.Completed
            )}
        }
    }

    /**
     * BLE 기기 스캔 및 "ACCESS" 쓰기
     * android_mvp의 AuthenticationActivity 로직 기반
     */
    fun scanBleDevice() {
        viewModelScope.launch {
            _state.update { it.copy(bleStatus = MfaStepStatus.InProgress) }

            bleManager.scanForDevices()
                .take(1) // 첫 번째 기기만
                .catch { e ->
                    _state.update { it.copy(
                        bleStatus = MfaStepStatus.Failed(e.message ?: "BLE scan failed")
                    )}
                }
                .collect { device ->
                    // GATT 연결 및 "ACCESS" 쓰기
                    bleManager.connectAndWriteAccess(device)
                        .catch { e ->
                            _state.update { it.copy(
                                bleStatus = MfaStepStatus.Failed(e.message ?: "BLE connection failed")
                            )}
                        }
                        .collect { success ->
                            if (success) {
                                _state.update { it.copy(
                                    bleConnected = true,
                                    bleStatus = MfaStepStatus.Completed
                                )}
                            } else {
                                _state.update { it.copy(
                                    bleStatus = MfaStepStatus.Failed("Failed to write ACCESS")
                                )}
                            }
                        }
                }
        }
    }

    /**
     * NFC Reader Mode 활성화
     * NFC 태그에서 UID 읽기 (차량 NFC 태그)
     * android_mvp의 NfcAuthActivity 로직 기반
     */
    fun enableNfcReaderMode(activity: Activity, carId: String) {
        viewModelScope.launch {
            _state.update { it.copy(nfcStatus = MfaStepStatus.InProgress) }

            nfcManager.enableReaderMode(activity)
                .take(1) // 첫 번째 태그만
                .catch { e ->
                    _state.update { it.copy(
                        nfcStatus = MfaStepStatus.Failed(e.message ?: "NFC read failed")
                    )}
                }
                .collect { tagResult ->
                    val nfcTagUid = tagResult.uid
                    _state.update { it.copy(
                        nfcTagUid = nfcTagUid,
                        nfcStatus = MfaStepStatus.Completed
                    )}

                    // NFC 태그 읽기 완료 → 서버 검증
                    verifyMfaWithServer(nfcTagUid, carId)
                }
        }
    }

    /**
     * 서버 MFA 검증
     * POST /auth/nfc/verify 호출
     * android_mvp의 NfcAuthActivity.verifyWithServer 로직 기반
     */
    private fun verifyMfaWithServer(nfcTagUid: String, carId: String) {
        viewModelScope.launch {
            _state.update { it.copy(isLoading = true) }

            try {
                // TokenManager에서 user_id 가져오기
                val userId = tokenManager.getUserId()
                if (userId == null) {
                    _state.update { it.copy(
                        isLoading = false,
                        error = "User ID not found. Please register face first."
                    )}
                    return@launch
                }

                // MFA 검증
                authenticateMfaUseCase(userId, nfcTagUid, carId)
                    .catch { e ->
                        _state.update { it.copy(
                            isLoading = false,
                            error = e.message ?: "MFA verification failed"
                        )}
                    }
                    .collect { result ->
                        if (result.serverVerified) {
                            _state.update { it.copy(
                                isLoading = false,
                                authSuccess = true,
                                sessionId = result.token
                            )}
                        } else {
                            _state.update { it.copy(
                                isLoading = false,
                                error = "MFA verification failed"
                            )}
                        }
                    }
            } catch (e: Exception) {
                _state.update { it.copy(
                    isLoading = false,
                    error = e.message ?: "Unknown error"
                )}
            }
        }
    }

    /**
     * 에러 초기화
     */
    fun clearError() {
        _state.update { it.copy(error = null) }
    }

    /**
     * 특정 단계 재시도
     */
    fun retryStep(step: String) {
        when (step) {
            "face" -> _state.update { it.copy(
                faceStatus = MfaStepStatus.Pending,
                faceRegistered = false
            )}
            "ble" -> _state.update { it.copy(
                bleStatus = MfaStepStatus.Pending,
                bleConnected = false
            )}
            "nfc" -> _state.update { it.copy(
                nfcStatus = MfaStepStatus.Pending,
                nfcTagUid = null
            )}
        }
    }
}
