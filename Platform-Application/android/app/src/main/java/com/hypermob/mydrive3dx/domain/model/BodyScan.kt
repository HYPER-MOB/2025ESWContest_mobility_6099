package com.hypermob.mydrive3dx.domain.model

/**
 * Body Scan Data
 *
 * 3D 카메라로 측정한 체형 데이터 (7지표)
 */
data class BodyScan(
    val scanId: String,
    val userId: String,
    val timestamp: String,

    // 체형 7지표
    val sittingHeight: Double,      // 앉은 키 (cm)
    val shoulderWidth: Double,       // 어깨 너비 (cm)
    val armLength: Double,           // 팔 길이 (cm)
    val headHeight: Double,          // 머리 높이 (cm)
    val eyeHeight: Double,           // 눈 높이 (cm)
    val legLength: Double,           // 다리 길이 (cm)
    val torsoLength: Double,         // 상체 길이 (cm)

    // 추가 정보
    val weight: Double? = null,      // 체중 (kg, 선택적)
    val height: Double? = null       // 전체 키 (cm, 선택적)
)

/**
 * Body Profile
 *
 * 바디스캔 결과로 생성된 사용자 프로필
 */
data class BodyProfile(
    val profileId: String,
    val userId: String,
    val userName: String,
    val bodyScan: BodyScan,
    val vehicleSettings: VehicleSettings,
    val createdAt: String,
    val updatedAt: String,
    val isActive: Boolean = true
)

/**
 * Vehicle Settings
 *
 * AI 예측으로 계산된 차량 세팅 값
 */
data class VehicleSettings(
    // 좌석 4축
    val seatFrontBack: Double,       // 좌석 전후 위치 (0-100)
    val seatUpDown: Double,          // 좌석 상하 위치 (0-100)
    val seatBackAngle: Double,       // 등받이 각도 (0-100)
    val seatHeadrestHeight: Double,  // 헤드레스트 높이 (0-100)

    // 미러 2축
    val leftMirrorAngle: Double,     // 좌측 미러 각도 (0-100)
    val rightMirrorAngle: Double,    // 우측 미러 각도 (0-100)

    // 핸들 2축
    val steeringWheelHeight: Double, // 핸들 높이 (0-100)
    val steeringWheelDepth: Double,  // 핸들 깊이 (0-100)

    // 차종별 보정 정보
    val vehicleModel: String? = null,
    val calibrationVersion: String = "1.0"
)

/**
 * Body Scan Request
 *
 * 바디스캔 시작 요청
 */
data class BodyScanRequest(
    val userId: String,
    val vehicleId: String? = null    // 특정 차량에 대한 스캔인 경우
)

/**
 * Vehicle Setting Application Result
 *
 * 차량 세팅 적용 결과
 */
data class VehicleSettingResult(
    val success: Boolean,
    val profileId: String,
    val appliedSettings: VehicleSettings,
    val estimatedTime: Int,          // 세팅 완료까지 예상 시간 (초)
    val message: String? = null
)
