package com.hypermob.mydrive3dx.domain.model

import kotlinx.serialization.Contextual
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.JsonElement

/**
 * Apply Settings Request
 * 차량 설정 적용 요청
 */
@Serializable
data class ApplySettingsRequest(
    val user_id: String,
    val apply_settings: Boolean = true
)

/**
 * Apply Settings Response
 * 차량 설정 적용 응답
 */
@Serializable
data class ApplySettingsResponse(
    val success: Boolean,
    val message: String,
    val applied_settings: Map<String, @Contextual Any>
)

/**
 * Settings History Item
 * 설정 히스토리 항목
 */
@Serializable
data class SettingsHistoryItem(
    val id: Long,
    val user_id: String,
    val car_id: String,
    val settings_applied: Map<String, @Contextual Any>,
    val adjustment_type: String,  // "auto" or "manual"
    val timestamp: String
)
