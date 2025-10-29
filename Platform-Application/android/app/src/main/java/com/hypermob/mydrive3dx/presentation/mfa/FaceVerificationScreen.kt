package com.hypermob.mydrive3dx.presentation.mfa

import androidx.camera.view.PreviewView
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.CheckCircle
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalLifecycleOwner
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.hilt.navigation.compose.hiltViewModel
import com.google.accompanist.permissions.ExperimentalPermissionsApi
import com.google.accompanist.permissions.isGranted
import com.google.accompanist.permissions.rememberPermissionState
import com.hypermob.mydrive3dx.data.hardware.FaceDetectionManager

/**
 * Face Verification Screen
 *
 * CameraX + ML Kit을 사용한 실제 얼굴 인증 화면
 */
@OptIn(ExperimentalMaterial3Api::class, ExperimentalPermissionsApi::class)
@Composable
fun FaceVerificationScreen(
    viewModel: MfaViewModel = hiltViewModel(),
    faceDetectionManager: FaceDetectionManager,
    onVerificationComplete: () -> Unit,
    onNavigateBack: () -> Unit
) {
    val context = LocalContext.current
    val lifecycleOwner = LocalLifecycleOwner.current
    val cameraPermissionState = rememberPermissionState(
        android.Manifest.permission.CAMERA
    )

    var faceDetectionResult by remember { mutableStateOf<FaceDetectionManager.FaceDetectionResult>(FaceDetectionManager.FaceDetectionResult.NoFace) }
    var isVerified by remember { mutableStateOf(false) }
    var detectionStarted by remember { mutableStateOf(false) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("얼굴 인증") },
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "뒤로")
                    }
                }
            )
        }
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
        ) {
            if (!cameraPermissionState.status.isGranted) {
                // 권한 요청 화면
                Box(
                    modifier = Modifier.fillMaxSize(),
                    contentAlignment = Alignment.Center
                ) {
                    Column(
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.spacedBy(16.dp),
                        modifier = Modifier.padding(16.dp)
                    ) {
                        Text(
                            text = "얼굴 인증을 위해 카메라 권한이 필요합니다",
                            style = MaterialTheme.typography.bodyLarge
                        )
                        Button(onClick = { cameraPermissionState.launchPermissionRequest() }) {
                            Text("권한 허용")
                        }
                    }
                }
            } else {
                // 카메라 프리뷰
                val context = LocalContext.current
                val previewView = remember {
                    PreviewView(context).apply {
                        scaleType = PreviewView.ScaleType.FILL_CENTER
                    }
                }

                LaunchedEffect(Unit) {
                    faceDetectionManager.startFaceDetection(lifecycleOwner, previewView)
                        .collect { result ->
                            faceDetectionResult = result

                            // 얼굴이 감지되고 confidence가 충분히 높으면 인증 성공
                            if (result is FaceDetectionManager.FaceDetectionResult.FaceDetected) {
                                if (result.confidence > 20f && !isVerified) {
                                    isVerified = true
                                    viewModel.setFaceVerified()
                                    kotlinx.coroutines.delay(1000)
                                    onVerificationComplete()
                                }
                            }
                        }
                }

                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .weight(1f)
                ) {
                    AndroidView(
                        factory = { previewView },
                        modifier = Modifier.fillMaxSize()
                    )

                    // 오버레이 UI
                    Column(
                        modifier = Modifier
                            .fillMaxSize()
                            .padding(24.dp),
                        verticalArrangement = Arrangement.SpaceBetween
                    ) {
                        // 상단 안내 메시지
                        Card(
                            colors = CardDefaults.cardColors(
                                containerColor = MaterialTheme.colorScheme.surface.copy(alpha = 0.9f)
                            )
                        ) {
                            Column(
                                modifier = Modifier.padding(16.dp),
                                verticalArrangement = Arrangement.spacedBy(8.dp)
                            ) {
                                Text(
                                    text = "얼굴을 카메라에 맞춰주세요",
                                    style = MaterialTheme.typography.titleMedium
                                )
                                when (faceDetectionResult) {
                                    is FaceDetectionManager.FaceDetectionResult.NoFace -> {
                                        Text(
                                            text = "얼굴이 감지되지 않았습니다",
                                            style = MaterialTheme.typography.bodyMedium,
                                            color = MaterialTheme.colorScheme.error
                                        )
                                    }
                                    is FaceDetectionManager.FaceDetectionResult.FaceDetected -> {
                                        val result = faceDetectionResult as FaceDetectionManager.FaceDetectionResult.FaceDetected
                                        Text(
                                            text = "얼굴 감지됨 (${result.faceCount}명, 신뢰도: ${result.confidence.toInt()}%)",
                                            style = MaterialTheme.typography.bodyMedium,
                                            color = MaterialTheme.colorScheme.primary
                                        )
                                    }
                                    is FaceDetectionManager.FaceDetectionResult.Error -> {
                                        val error = faceDetectionResult as FaceDetectionManager.FaceDetectionResult.Error
                                        Text(
                                            text = "오류: ${error.message}",
                                            style = MaterialTheme.typography.bodySmall,
                                            color = MaterialTheme.colorScheme.error
                                        )
                                    }
                                }
                            }
                        }

                        // 하단 인증 완료 표시
                        if (isVerified) {
                            Card(
                                colors = CardDefaults.cardColors(
                                    containerColor = MaterialTheme.colorScheme.primaryContainer
                                )
                            ) {
                                Row(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .padding(16.dp),
                                    horizontalArrangement = Arrangement.Center,
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    Icon(
                                        Icons.Default.CheckCircle,
                                        contentDescription = null,
                                        tint = MaterialTheme.colorScheme.primary,
                                        modifier = Modifier.size(32.dp)
                                    )
                                    Spacer(modifier = Modifier.width(12.dp))
                                    Text(
                                        text = "얼굴 인증 완료!",
                                        style = MaterialTheme.typography.titleLarge,
                                        color = MaterialTheme.colorScheme.onPrimaryContainer
                                    )
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Cleanup
    DisposableEffect(Unit) {
        onDispose {
            if (detectionStarted) {
                faceDetectionManager.stopFaceDetection()
            }
        }
    }
}
