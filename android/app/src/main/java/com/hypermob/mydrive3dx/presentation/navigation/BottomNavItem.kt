package com.hypermob.mydrive3dx.presentation.navigation

import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.ui.graphics.vector.ImageVector

/**
 * Bottom Navigation Items
 *
 * 하단 네비게이션 바 아이템 정의
 */
data class BottomNavItem(
    val screen: Screen,
    val label: String,
    val icon: ImageVector
)

val bottomNavItems = listOf(
    BottomNavItem(
        screen = Screen.Home,
        label = "홈",
        icon = Icons.Default.Home
    ),
    BottomNavItem(
        screen = Screen.Rental,
        label = "대여",
        icon = Icons.Default.DirectionsCar
    ),
    BottomNavItem(
        screen = Screen.Control,
        label = "제어",
        icon = Icons.Default.Phonelink
    ),
    BottomNavItem(
        screen = Screen.Settings,
        label = "설정",
        icon = Icons.Default.Settings
    )
)
