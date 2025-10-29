package com.hypermob.mydrive3dx.presentation.drowsiness

import androidx.camera.view.PreviewView
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalLifecycleOwner
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.hilt.navigation.compose.hiltViewModel
import com.google.accompanist.permissions.ExperimentalPermissionsApi
import com.google.accompanist.permissions.isGranted
import com.google.accompanist.permissions.rememberPermissionState
import com.hypermob.mydrive3dx.data.hardware.DrowsinessDetectionManager
import com.hypermob.mydrive3dx.domain.model.DrowsinessLevel
import java.util.concurrent.TimeUnit

/**
 * Drowsiness Monitor Screen
 *
 * 졸음 감지 모니터링 화면
 */
@OptIn(ExperimentalMaterial3Api::class, ExperimentalPermissionsApi::class)
@Composable
fun DrowsinessMonitorScreen(
    viewModel: DrowsinessMonitorViewModel = hiltViewModel(),
    drowsinessDetectionManager: DrowsinessDetectionManager,
    onNavigateBack: () -> Unit
) {
    val uiState by viewModel.uiState.collectAsState()
    val lifecycleOwner = LocalLifecycleOwner.current
    val cameraPermissionState = rememberPermissionState(android.Manifest.permission.CAMERA)

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("졸음 감지 모니터") },
                navigationIcon = {
                    IconButton(onClick = {
                        if (uiState.isMonitoring) {
                            viewModel.stopMonitoring()
                        }
                        onNavigateBack()
                    }) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "뒤로")
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = getLevelColor(uiState.currentLevel)
                )
            )
        }
    ) { paddingValues ->
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
        ) {
            if (!uiState.isMonitoring) {
                // 대기 화면
                StandbyScreen(
                    onStartMonitoring = {
                        if (cameraPermissionState.status.isGranted) {
                            viewModel.startMonitoring()
                        } else {
                            cameraPermissionState.launchPermissionRequest()
                        }
                    }
                )
            } else {
                if (!cameraPermissionState.status.isGranted) {
                    PermissionRequiredScreen(
                        onRequestPermission = { cameraPermissionState.launchPermissionRequest() }
                    )
                } else {
                    MonitoringScreen(
                        drowsinessDetectionManager = drowsinessDetectionManager,
                        lifecycleOwner = lifecycleOwner,
                        viewModel = viewModel,
                        uiState = uiState
                    )
                }
            }

            // 경고 다이얼로그
            if (uiState.showAlert && uiState.alertMessage != null) {
                AlertDialog(
                    onDismissRequest = { viewModel.dismissAlert() },
                    icon = {
                        Icon(
                            imageVector = getAlertIcon(uiState.currentLevel),
                            contentDescription = null,
                            tint = getAlertIconColor(uiState.currentLevel),
                            modifier = Modifier.size(48.dp)
                        )
                    },
                    title = {
                        Text(
                            text = getAlertTitle(uiState.currentLevel),
                            style = MaterialTheme.typography.headlineSmall
                        )
                    },
                    text = {
                        Text(
                            text = uiState.alertMessage!!,
                            style = MaterialTheme.typography.bodyLarge
                        )
                    },
                    confirmButton = {
                        Button(
                            onClick = { viewModel.dismissAlert() },
                            colors = ButtonDefaults.buttonColors(
                                containerColor = getAlertIconColor(uiState.currentLevel)
                            )
                        ) {
                            Text("확인")
                        }
                    }
                )
            }

            // 에러 메시지
            if (uiState.errorMessage != null) {
                Snackbar(
                    modifier = Modifier
                        .align(Alignment.BottomCenter)
                        .padding(16.dp)
                ) {
                    Text(uiState.errorMessage!!)
                }
            }
        }
    }
}

/**
 * 대기 화면
 */
@Composable
private fun StandbyScreen(onStartMonitoring: () -> Unit) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(24.dp)
    ) {
        Spacer(modifier = Modifier.weight(1f))

        Icon(
            Icons.Default.RemoveRedEye,
            contentDescription = null,
            modifier = Modifier.size(120.dp),
            tint = MaterialTheme.colorScheme.primary
        )

        Text(
            text = "졸음 감지 모니터",
            style = MaterialTheme.typography.headlineMedium,
            color = MaterialTheme.colorScheme.primary
        )

        Card {
            Column(
                modifier = Modifier.padding(16.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                Text(
                    text = "안전 운행을 위한 AI 졸음 감지",
                    style = MaterialTheme.typography.titleMedium
                )
                FeatureItem("실시간 얼굴 인식 및 눈 추적")
                FeatureItem("머리 자세 분석")
                FeatureItem("눈 깜빡임 빈도 측정")
                FeatureItem("3단계 경고 시스템")
            }
        }

        Card(
            colors = CardDefaults.cardColors(
                containerColor = MaterialTheme.colorScheme.errorContainer
            )
        ) {
            Row(
                modifier = Modifier.padding(16.dp),
                horizontalArrangement = Arrangement.spacedBy(12.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Icon(
                    Icons.Default.Warning,
                    contentDescription = null,
                    tint = MaterialTheme.colorScheme.error
                )
                Text(
                    text = "운전 중에만 사용하세요",
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onErrorContainer
                )
            }
        }

        Spacer(modifier = Modifier.weight(1f))

        Button(
            onClick = onStartMonitoring,
            modifier = Modifier.fillMaxWidth()
        ) {
            Icon(Icons.Default.PlayArrow, contentDescription = null)
            Spacer(modifier = Modifier.width(8.dp))
            Text("모니터링 시작")
        }
    }
}

@Composable
private fun FeatureItem(text: String) {
    Row(
        verticalAlignment = Alignment.Top,
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        Icon(
            Icons.Default.Check,
            contentDescription = null,
            modifier = Modifier.size(20.dp),
            tint = MaterialTheme.colorScheme.primary
        )
        Text(
            text = text,
            style = MaterialTheme.typography.bodyMedium
        )
    }
}

/**
 * 권한 요청 화면
 */
@Composable
private fun PermissionRequiredScreen(onRequestPermission: () -> Unit) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        Icon(
            Icons.Default.CameraAlt,
            contentDescription = null,
            modifier = Modifier.size(80.dp),
            tint = MaterialTheme.colorScheme.onSurfaceVariant
        )
        Spacer(modifier = Modifier.height(16.dp))
        Text(
            text = "카메라 권한이 필요합니다",
            style = MaterialTheme.typography.titleLarge
        )
        Spacer(modifier = Modifier.height(8.dp))
        Text(
            text = "졸음 감지를 위해 카메라 접근 권한이 필요합니다",
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )
        Spacer(modifier = Modifier.height(24.dp))
        Button(onClick = onRequestPermission) {
            Text("권한 허용")
        }
    }
}

/**
 * 모니터링 화면
 */
@Composable
private fun MonitoringScreen(
    drowsinessDetectionManager: DrowsinessDetectionManager,
    lifecycleOwner: androidx.lifecycle.LifecycleOwner,
    viewModel: DrowsinessMonitorViewModel,
    uiState: DrowsinessMonitorUiState
) {
    val context = LocalContext.current
    val previewView = remember {
        PreviewView(context).apply {
            scaleType = PreviewView.ScaleType.FILL_CENTER
        }
    }

    LaunchedEffect(Unit) {
        drowsinessDetectionManager.startDrowsinessDetection(
            lifecycleOwner,
            previewView
        ).collect { result ->
            viewModel.onDetectionResult(result)
        }
    }

    Box(modifier = Modifier.fillMaxSize()) {
        // 카메라 프리뷰
        AndroidView(
            factory = { previewView },
            modifier = Modifier.fillMaxSize()
        )

        // 오버레이 UI
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(16.dp),
            verticalArrangement = Arrangement.SpaceBetween
        ) {
            // 상단: 실시간 메트릭
            Column(
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                // 현재 상태
                StatusCard(uiState.currentLevel)

                // 세부 메트릭
                if (uiState.detectionResult != null) {
                    MetricsCard(uiState)
                }
            }

            // 하단: 통계 및 컨트롤
            Column(
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                StatisticsCard(uiState)

                StopButton(
                    onClick = { viewModel.stopMonitoring() }
                )
            }
        }
    }

    DisposableEffect(Unit) {
        onDispose {
            drowsinessDetectionManager.stopDrowsinessDetection()
        }
    }
}

/**
 * 상태 카드
 */
@Composable
private fun StatusCard(level: DrowsinessLevel) {
    Card(
        colors = CardDefaults.cardColors(
            containerColor = getLevelColor(level)
        )
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            horizontalArrangement = Arrangement.spacedBy(12.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Icon(
                imageVector = getLevelIcon(level),
                contentDescription = null,
                modifier = Modifier.size(32.dp),
                tint = Color.White
            )
            Column {
                Text(
                    text = getLevelText(level),
                    style = MaterialTheme.typography.titleLarge,
                    fontWeight = FontWeight.Bold,
                    color = Color.White
                )
                Text(
                    text = getLevelDescription(level),
                    style = MaterialTheme.typography.bodyMedium,
                    color = Color.White.copy(alpha = 0.9f)
                )
            }
        }
    }
}

/**
 * 메트릭 카드
 */
@Composable
private fun MetricsCard(uiState: DrowsinessMonitorUiState) {
    val result = uiState.detectionResult ?: return

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
                text = "실시간 메트릭",
                style = MaterialTheme.typography.titleSmall,
                color = MaterialTheme.colorScheme.onSurface
            )

            MetricRow("눈 개방도 (EAR)", String.format("%.3f", result.eyeAspectRatio))
            MetricRow("눈 감은 시간", "${result.eyeClosureDuration}ms")
            MetricRow("머리 기울기", String.format("%.1f°", result.headPoseAngle))
            MetricRow("깜빡임 빈도", "${result.blinkFrequency}/min")
            MetricRow("신뢰도", "${(result.confidence * 100).toInt()}%")
        }
    }
}

@Composable
private fun MetricRow(label: String, value: String) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(
            text = label,
            style = MaterialTheme.typography.bodySmall,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )
        Text(
            text = value,
            style = MaterialTheme.typography.bodySmall,
            fontWeight = FontWeight.Medium,
            color = MaterialTheme.colorScheme.onSurface
        )
    }
}

/**
 * 통계 카드
 */
@Composable
private fun StatisticsCard(uiState: DrowsinessMonitorUiState) {
    Card(
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surface.copy(alpha = 0.9f)
        )
    ) {
        Column(
            modifier = Modifier.padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = "세션 통계",
                    style = MaterialTheme.typography.titleSmall,
                    color = MaterialTheme.colorScheme.onSurface
                )
                Text(
                    text = formatDuration(uiState.sessionStartTime),
                    style = MaterialTheme.typography.bodyMedium,
                    fontWeight = FontWeight.Medium,
                    color = MaterialTheme.colorScheme.primary
                )
            }

            Divider()

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceEvenly
            ) {
                AlertCountItem("경고", uiState.alertCount.warningCount, Color(0xFFFFA726))
                AlertCountItem("위험", uiState.alertCount.dangerCount, Color(0xFFFF7043))
                AlertCountItem("심각", uiState.alertCount.criticalCount, Color(0xFFE53935))
            }
        }
    }
}

@Composable
private fun AlertCountItem(label: String, count: Int, color: Color) {
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(4.dp)
    ) {
        Text(
            text = count.toString(),
            style = MaterialTheme.typography.titleLarge,
            fontWeight = FontWeight.Bold,
            color = color
        )
        Text(
            text = label,
            style = MaterialTheme.typography.bodySmall,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )
    }
}

/**
 * 중지 버튼
 */
@Composable
private fun StopButton(onClick: () -> Unit) {
    Button(
        onClick = onClick,
        modifier = Modifier.fillMaxWidth(),
        colors = ButtonDefaults.buttonColors(
            containerColor = MaterialTheme.colorScheme.error
        )
    ) {
        Icon(Icons.Default.Stop, contentDescription = null)
        Spacer(modifier = Modifier.width(8.dp))
        Text("모니터링 중지")
    }
}

/**
 * 레벨별 색상
 */
@Composable
private fun getLevelColor(level: DrowsinessLevel): Color {
    return when (level) {
        DrowsinessLevel.NORMAL -> MaterialTheme.colorScheme.primary
        DrowsinessLevel.WARNING -> Color(0xFFFFA726)  // Orange
        DrowsinessLevel.DANGER -> Color(0xFFFF7043)   // Deep Orange
        DrowsinessLevel.CRITICAL -> Color(0xFFE53935) // Red
    }
}

/**
 * 레벨별 아이콘
 */
private fun getLevelIcon(level: DrowsinessLevel) = when (level) {
    DrowsinessLevel.NORMAL -> Icons.Default.CheckCircle
    DrowsinessLevel.WARNING -> Icons.Default.Warning
    DrowsinessLevel.DANGER -> Icons.Default.Error
    DrowsinessLevel.CRITICAL -> Icons.Default.Dangerous
}

/**
 * 레벨별 텍스트
 */
private fun getLevelText(level: DrowsinessLevel) = when (level) {
    DrowsinessLevel.NORMAL -> "정상"
    DrowsinessLevel.WARNING -> "경고"
    DrowsinessLevel.DANGER -> "위험"
    DrowsinessLevel.CRITICAL -> "심각"
}

/**
 * 레벨별 설명
 */
private fun getLevelDescription(level: DrowsinessLevel) = when (level) {
    DrowsinessLevel.NORMAL -> "정상적인 운전 상태입니다"
    DrowsinessLevel.WARNING -> "졸음 징후가 감지되었습니다"
    DrowsinessLevel.DANGER -> "위험! 휴식이 필요합니다"
    DrowsinessLevel.CRITICAL -> "매우 위험! 즉시 차량을 정차하세요"
}

/**
 * 경고 다이얼로그 아이콘
 */
private fun getAlertIcon(level: DrowsinessLevel) = when (level) {
    DrowsinessLevel.WARNING -> Icons.Default.Warning
    DrowsinessLevel.DANGER -> Icons.Default.Error
    DrowsinessLevel.CRITICAL -> Icons.Default.Dangerous
    else -> Icons.Default.Info
}

/**
 * 경고 다이얼로그 아이콘 색상
 */
@Composable
private fun getAlertIconColor(level: DrowsinessLevel) = when (level) {
    DrowsinessLevel.WARNING -> Color(0xFFFFA726)
    DrowsinessLevel.DANGER -> Color(0xFFFF7043)
    DrowsinessLevel.CRITICAL -> Color(0xFFE53935)
    else -> MaterialTheme.colorScheme.primary
}

/**
 * 경고 다이얼로그 제목
 */
private fun getAlertTitle(level: DrowsinessLevel) = when (level) {
    DrowsinessLevel.WARNING -> "졸음 경고"
    DrowsinessLevel.DANGER -> "위험 경고"
    DrowsinessLevel.CRITICAL -> "심각 경고"
    else -> "알림"
}

/**
 * 세션 지속시간 포맷
 */
@Composable
private fun formatDuration(startTime: Long?): String {
    if (startTime == null) return "00:00"

    val duration = System.currentTimeMillis() - startTime
    val minutes = TimeUnit.MILLISECONDS.toMinutes(duration)
    val seconds = TimeUnit.MILLISECONDS.toSeconds(duration) % 60

    return String.format("%02d:%02d", minutes, seconds)
}
