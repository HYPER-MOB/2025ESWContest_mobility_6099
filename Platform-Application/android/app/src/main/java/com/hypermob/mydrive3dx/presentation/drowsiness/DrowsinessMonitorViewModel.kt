package com.hypermob.mydrive3dx.presentation.drowsiness

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.hypermob.mydrive3dx.domain.model.DrowsinessDetectionResult
import com.hypermob.mydrive3dx.domain.model.DrowsinessLevel
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import javax.inject.Inject

/**
 * Drowsiness Monitor ViewModel
 *
 * 졸음 감지 모니터링 로직
 */
@HiltViewModel
class DrowsinessMonitorViewModel @Inject constructor(
    // TODO: UseCase 추가 (이벤트 로그 저장 등)
) : ViewModel() {

    private val _uiState = MutableStateFlow(DrowsinessMonitorUiState())
    val uiState: StateFlow<DrowsinessMonitorUiState> = _uiState.asStateFlow()

    private var lastAlertTime = 0L
    private val alertCooldown = 10000L // 10초 쿨다운

    /**
     * 모니터링 시작
     */
    fun startMonitoring() {
        _uiState.update {
            it.copy(
                isMonitoring = true,
                sessionStartTime = System.currentTimeMillis(),
                errorMessage = null
            )
        }
    }

    /**
     * 모니터링 중지
     */
    fun stopMonitoring() {
        _uiState.update {
            it.copy(
                isMonitoring = false,
                sessionStartTime = null
            )
        }
    }

    /**
     * 감지 결과 업데이트
     */
    fun onDetectionResult(result: DrowsinessDetectionResult) {
        val currentTime = System.currentTimeMillis()
        val previousLevel = _uiState.value.currentLevel

        _uiState.update {
            it.copy(
                detectionResult = result,
                currentLevel = result.level
            )
        }

        // 레벨이 변경되고 NORMAL이 아닌 경우 경고
        if (result.level != DrowsinessLevel.NORMAL && result.level != previousLevel) {
            incrementAlertCount(result.level)

            // 쿨다운 체크
            if (currentTime - lastAlertTime > alertCooldown) {
                showAlert(result.level, result.message ?: "")
                lastAlertTime = currentTime

                // 실제 조치 실행
                executeAction(result.level)
            }
        }
    }

    /**
     * 경고 횟수 증가
     */
    private fun incrementAlertCount(level: DrowsinessLevel) {
        _uiState.update {
            val newAlertCount = when (level) {
                DrowsinessLevel.WARNING -> it.alertCount.copy(warningCount = it.alertCount.warningCount + 1)
                DrowsinessLevel.DANGER -> it.alertCount.copy(dangerCount = it.alertCount.dangerCount + 1)
                DrowsinessLevel.CRITICAL -> it.alertCount.copy(criticalCount = it.alertCount.criticalCount + 1)
                else -> it.alertCount
            }
            it.copy(alertCount = newAlertCount)
        }
    }

    /**
     * 경고 표시
     */
    private fun showAlert(level: DrowsinessLevel, message: String) {
        _uiState.update {
            it.copy(
                showAlert = true,
                alertMessage = message
            )
        }
    }

    /**
     * 경고 닫기
     */
    fun dismissAlert() {
        _uiState.update {
            it.copy(
                showAlert = false,
                alertMessage = null
            )
        }
    }

    /**
     * 조치 실행
     */
    private fun executeAction(level: DrowsinessLevel) {
        viewModelScope.launch {
            when (level) {
                DrowsinessLevel.WARNING -> {
                    // Level 1: 시트 진동 + 음성 경고
                    // TODO: 시트 진동 API 호출
                    // TODO: 음성 경고 재생
                }
                DrowsinessLevel.DANGER -> {
                    // Level 2: HUD 팝업
                    // TODO: HUD 팝업 표시
                }
                DrowsinessLevel.CRITICAL -> {
                    // Level 3: SOS 문자 + 비상등
                    // TODO: SOS 문자 발송
                    // TODO: 비상등 켜기
                }
                else -> {}
            }
        }
    }

    /**
     * 에러 처리
     */
    fun onError(message: String) {
        _uiState.update {
            it.copy(
                errorMessage = message,
                isMonitoring = false
            )
        }
    }

    /**
     * 세션 통계 조회
     */
    fun getSessionDuration(): Long {
        val startTime = _uiState.value.sessionStartTime ?: return 0L
        return System.currentTimeMillis() - startTime
    }

    /**
     * 초기화
     */
    fun reset() {
        _uiState.value = DrowsinessMonitorUiState()
        lastAlertTime = 0L
    }
}
