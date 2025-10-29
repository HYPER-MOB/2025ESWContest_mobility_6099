package com.hypermob.mydrive3dx.presentation.drowsiness

import com.hypermob.mydrive3dx.domain.model.DrowsinessDetectionResult
import com.hypermob.mydrive3dx.domain.model.DrowsinessLevel

/**
 * Drowsiness Monitor UI State
 */
data class DrowsinessMonitorUiState(
    val isMonitoring: Boolean = false,
    val currentLevel: DrowsinessLevel = DrowsinessLevel.NORMAL,
    val detectionResult: DrowsinessDetectionResult? = null,
    val alertCount: AlertCount = AlertCount(),
    val sessionStartTime: Long? = null,
    val showAlert: Boolean = false,
    val alertMessage: String? = null,
    val errorMessage: String? = null
)

/**
 * Alert Count
 *
 * 세션별 경고 횟수
 */
data class AlertCount(
    val warningCount: Int = 0,
    val dangerCount: Int = 0,
    val criticalCount: Int = 0
) {
    val totalCount: Int
        get() = warningCount + dangerCount + criticalCount
}
