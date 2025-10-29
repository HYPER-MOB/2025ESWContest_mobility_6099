package com.hypermob.mydrive3dx

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import com.hypermob.mydrive3dx.data.hardware.BodyScanManager
import com.hypermob.mydrive3dx.data.hardware.DrowsinessDetectionManager
import com.hypermob.mydrive3dx.data.local.TokenManager
import com.hypermob.mydrive3dx.presentation.navigation.MainScreen
import com.hypermob.mydrive3dx.presentation.navigation.Screen
import com.hypermob.mydrive3dx.presentation.theme.MyDrive3DXTheme
import dagger.hilt.android.AndroidEntryPoint
import javax.inject.Inject

/**
 * MainActivity
 *
 * Jetpack Compose 진입점
 */
@AndroidEntryPoint
class MainActivity : ComponentActivity() {

    @Inject
    lateinit var bodyScanManager: BodyScanManager

    @Inject
    lateinit var drowsinessDetectionManager: DrowsinessDetectionManager

    @Inject
    lateinit var faceDetectionManager: com.hypermob.mydrive3dx.data.hardware.FaceDetectionManager

    @Inject
    lateinit var tokenManager: TokenManager

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        setContent {
            MyDrive3DXTheme {
                // 자동 로그인: 토큰이 있으면 Home, 없으면 Login
                val isLoggedIn by tokenManager.isLoggedIn().collectAsState(initial = false)

                MainScreen(
                    startDestination = if (isLoggedIn) Screen.Home.route else Screen.Login.route,
                    bodyScanManager = bodyScanManager,
                    drowsinessDetectionManager = drowsinessDetectionManager,
                    faceDetectionManager = faceDetectionManager
                )
            }
        }
    }
}
