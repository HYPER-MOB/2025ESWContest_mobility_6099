package com.hypermob.mydrive3dx.presentation.theme

import android.app.Activity
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.SideEffect
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.platform.LocalView
import androidx.core.view.WindowCompat

/**
 * MyDrive3DX Dark Color Scheme (웹 프로토타입 기준)
 */
private val MyDriveDarkColorScheme = darkColorScheme(
    // Primary Colors
    primary = PrimaryCyan,
    onPrimary = DarkBackground,  // Primary 위의 텍스트 색상
    primaryContainer = PrimaryCyanGlow,
    onPrimaryContainer = DarkBackground,

    // Secondary Colors
    secondary = DarkSecondary,
    onSecondary = TextPrimary,
    secondaryContainer = DarkCard,
    onSecondaryContainer = TextSecondary,

    // Tertiary Colors (Accent)
    tertiary = PrimaryCyanGlow,
    onTertiary = DarkBackground,
    tertiaryContainer = DarkSecondary,
    onTertiaryContainer = TextPrimary,

    // Background & Surface
    background = DarkBackground,
    onBackground = TextPrimary,
    surface = DarkCard,
    onSurface = TextPrimary,
    surfaceVariant = DarkSecondary,
    onSurfaceVariant = TextSecondary,

    // Error
    error = ErrorRed,
    onError = TextPrimary,
    errorContainer = ErrorRed,
    onErrorContainer = TextPrimary,

    // Outline (Border)
    outline = Border,
    outlineVariant = BorderLight
)

/**
 * MyDrive3DX Theme
 *
 * @param darkTheme 다크 테마 사용 여부 (기본값: 시스템 설정 따름)
 * @param content Composable 컨텐츠
 */
@Composable
fun MyDrive3DXTheme(
    darkTheme: Boolean = isSystemInDarkTheme(),
    content: @Composable () -> Unit
) {
    // 현재는 Dark Theme만 지원 (웹 프로토타입과 일치)
    val colorScheme = MyDriveDarkColorScheme

    val view = LocalView.current
    if (!view.isInEditMode) {
        SideEffect {
            val window = (view.context as Activity).window
            window.statusBarColor = colorScheme.background.toArgb()
            window.navigationBarColor = colorScheme.background.toArgb()
            WindowCompat.getInsetsController(window, view).apply {
                isAppearanceLightStatusBars = false
                isAppearanceLightNavigationBars = false
            }
        }
    }

    MaterialTheme(
        colorScheme = colorScheme,
        typography = MyDriveTypography,
        content = content
    )
}
