package com.hypermob.mydrive3dx.presentation.navigation

import androidx.compose.foundation.layout.padding
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.navigation.NavDestination.Companion.hierarchy
import androidx.navigation.NavGraph.Companion.findStartDestination
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import com.hypermob.mydrive3dx.data.hardware.BodyScanManager
import com.hypermob.mydrive3dx.data.hardware.DrowsinessDetectionManager
import com.hypermob.mydrive3dx.data.hardware.FaceDetectionManager

/**
 * Main Screen with Bottom Navigation
 *
 * 하단 네비게이션 바를 포함한 메인 화면
 */
@Composable
fun MainScreen(
    startDestination: String = Screen.Login.route,
    bodyScanManager: BodyScanManager,
    drowsinessDetectionManager: DrowsinessDetectionManager,
    faceDetectionManager: FaceDetectionManager
) {
    val navController = rememberNavController()
    val navBackStackEntry by navController.currentBackStackEntryAsState()
    val currentDestination = navBackStackEntry?.destination

    // Bottom Navigation을 표시할 화면들
    val bottomNavScreens = bottomNavItems.map { it.screen.route }
    val showBottomBar = currentDestination?.route in bottomNavScreens

    Scaffold(
        bottomBar = {
            if (showBottomBar) {
                NavigationBar {
                    bottomNavItems.forEach { item ->
                        NavigationBarItem(
                            icon = {
                                Icon(
                                    imageVector = item.icon,
                                    contentDescription = item.label
                                )
                            },
                            label = { Text(item.label) },
                            selected = currentDestination?.hierarchy?.any {
                                it.route == item.screen.route
                            } == true,
                            onClick = {
                                navController.navigate(item.screen.route) {
                                    // Pop up to the start destination
                                    popUpTo(navController.graph.findStartDestination().id) {
                                        saveState = true
                                    }
                                    // Avoid multiple copies
                                    launchSingleTop = true
                                    // Restore state
                                    restoreState = true
                                }
                            }
                        )
                    }
                }
            }
        }
    ) { innerPadding ->
        NavGraph(
            navController = navController,
            startDestination = startDestination,
            bodyScanManager = bodyScanManager,
            drowsinessDetectionManager = drowsinessDetectionManager,
            faceDetectionManager = faceDetectionManager,
            modifier = Modifier.padding(innerPadding)
        )
    }
}
