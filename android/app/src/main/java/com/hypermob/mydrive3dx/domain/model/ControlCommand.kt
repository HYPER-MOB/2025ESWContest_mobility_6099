package com.hypermob.mydrive3dx.domain.model

/**
 * Control Command Type
 *
 * 차량 제어 커맨드 타입
 */
enum class ControlCommandType {
    LOCK_DOORS,
    UNLOCK_DOORS,
    START_ENGINE,
    STOP_ENGINE,
    TURN_ON_CLIMATE,
    TURN_OFF_CLIMATE,
    SET_CLIMATE_TEMPERATURE,
    HONK_HORN,
    FLASH_LIGHTS
}

/**
 * Control Command Request
 */
data class ControlCommandRequest(
    val rentalId: String,
    val commandType: ControlCommandType,
    val parameters: Map<String, Any>? = null // 예: temperature, fanSpeed 등
)

/**
 * Control Command Response
 */
data class ControlCommandResponse(
    val success: Boolean,
    val message: String?,
    val updatedControl: VehicleControl?
)
