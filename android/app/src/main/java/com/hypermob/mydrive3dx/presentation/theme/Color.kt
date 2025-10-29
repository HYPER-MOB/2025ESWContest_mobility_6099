package com.hypermob.mydrive3dx.presentation.theme

import androidx.compose.ui.graphics.Color

// Primary Colors (웹 프로토타입 기준)
val PrimaryCyan = Color(0xFF22D3EE)  // hsl(189 85% 55%) - 시안 블루
val PrimaryCyanGlow = Color(0xFF4FE0F3)  // hsl(189 95% 65%) - 밝은 시안

// Background Colors (Dark Theme)
val DarkBackground = Color(0xFF1A202C)  // hsl(222 47% 11%)
val DarkCard = Color(0xFF2D3748)  // hsl(222 47% 15%)
val DarkSecondary = Color(0xFF374151)  // hsl(217 33% 22%)

// Semantic Colors
val SuccessGreen = Color(0xFF10B981)  // hsl(142 76% 36%)
val WarningYellow = Color(0xFFFBBF24)  // hsl(38 92% 50%)
val ErrorRed = Color(0xFFEF4444)  // hsl(0 84% 60%)

// Text Colors
val TextPrimary = Color(0xFFF9FAFB)  // 거의 흰색
val TextSecondary = Color(0xFFD1D5DB)  // 회색
val TextTertiary = Color(0xFF9CA3AF)  // 어두운 회색

// Border & Divider
val Border = Color(0xFF4B5563)  // hsl(220 13% 32%)
val BorderLight = Color(0x80000000)  // 50% 투명 검정

// Status Badge Colors (웹 프로토타입 기준)
object StatusColors {
    // Rental Status
    val Requested = Color(0xFFFBBF24)  // Yellow
    val Approved = Color(0xFF10B981)  // Green
    val Picked = Color(0xFF22D3EE)  // Cyan
    val Returned = Color(0xFF9CA3AF)  // Gray
    val Canceled = Color(0xFFEF4444)  // Red

    // MFA Status
    val Pending = Color(0xFFFBBF24)  // Yellow
    val SmartKeyOk = Color(0xFF10B981)  // Green
    val FaceOk = Color(0xFF10B981)  // Green
    val NfcBtOk = Color(0xFF10B981)  // Green
    val Passed = Color(0xFF10B981)  // Green
    val Failed = Color(0xFFEF4444)  // Red

    // Safety Status
    val SafetyOk = Color(0xFF10B981)  // Green
    val SafetyBlock = Color(0xFFEF4444)  // Red
}
