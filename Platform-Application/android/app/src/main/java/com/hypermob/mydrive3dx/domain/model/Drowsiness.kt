package com.hypermob.mydrive3dx.domain.model

/**
 * Drowsiness Level
 *
 * 졸음 경고 단계
 */
enum class DrowsinessLevel {
    NORMAL,         // 정상
    WARNING,        // 경고 (Level 1: 시트 진동 + 음성 경고)
    DANGER,         // 위험 (Level 2: HUD 팝업)
    CRITICAL        // 심각 (Level 3: SOS 문자 + 비상등)
}

/**
 * Drowsiness Detection Result
 *
 * 졸음 감지 결과
 */
data class DrowsinessDetectionResult(
    val timestamp: Long,
    val level: DrowsinessLevel,
    val eyeAspectRatio: Float,           // EAR (Eye Aspect Ratio)
    val eyeClosureDuration: Long,        // 눈 감은 시간 (ms)
    val headPoseAngle: Float,            // 머리 기울기 각도
    val blinkFrequency: Int,             // 눈 깜빡임 빈도 (per minute)
    val confidence: Float,                // 감지 신뢰도
    val message: String? = null
)

/**
 * Drowsiness Event
 *
 * 졸음 이벤트 (로그용)
 */
data class DrowsinessEvent(
    val eventId: String,
    val userId: String,
    val rentalId: String?,
    val detectionResult: DrowsinessDetectionResult,
    val actionTaken: DrowsinessAction,
    val timestamp: String,
    val videoSnapshotUrl: String? = null    // 이벤트 발생 시 스냅샷
)

/**
 * Drowsiness Action
 *
 * 졸음 감지 시 취한 조치
 */
enum class DrowsinessAction {
    NONE,               // 조치 없음
    VIBRATION,          // 시트 진동
    VOICE_ALERT,        // 음성 경고
    HUD_POPUP,          // HUD 팝업
    SOS_ALERT,          // SOS 문자 발송
    EMERGENCY_LIGHTS    // 비상등 점등
}

/**
 * Drowsiness Statistics
 *
 * 졸음 통계 (주행 세션별)
 */
data class DrowsinessStatistics(
    val sessionId: String,
    val userId: String,
    val startTime: String,
    val endTime: String,
    val totalDrowsinessEvents: Int,
    val warningCount: Int,
    val dangerCount: Int,
    val criticalCount: Int,
    val averageEAR: Float,
    val totalDrivingDuration: Long          // 총 주행 시간 (ms)
)
