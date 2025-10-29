package com.hypermob.mydrive3dx.presentation.bodyscan

import android.content.Context
import android.graphics.Bitmap
import android.graphics.ImageDecoder
import android.os.Build
import android.provider.MediaStore
import androidx.camera.core.*
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.compose.foundation.shape.CircleShape
import androidx.core.content.ContextCompat
import java.io.File
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
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
import com.hypermob.mydrive3dx.data.hardware.BodyScanManager

/**
 * Body Scan Screen
 *
 * 3D 바디스캔 및 프로필 생성 화면
 */
@OptIn(ExperimentalMaterial3Api::class, ExperimentalPermissionsApi::class)
@Composable
fun BodyScanScreen(
    viewModel: BodyScanViewModel = hiltViewModel(),
    bodyScanManager: BodyScanManager,
    onNavigateBack: () -> Unit,
    onScanComplete: () -> Unit
) {
    val uiState by viewModel.uiState.collectAsState()
    val context = LocalContext.current
    val lifecycleOwner = LocalLifecycleOwner.current
    val cameraPermissionState = rememberPermissionState(android.Manifest.permission.CAMERA)

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("3D 바디스캔") },
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "뒤로")
                    }
                }
            )
        }
    ) { paddingValues ->
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
        ) {
            when (uiState.scanStep) {
                ScanStep.INSTRUCTION -> {
                    InstructionScreen(
                        onStartScan = {
                            if (cameraPermissionState.status.isGranted) {
                                viewModel.startScan()
                            } else {
                                cameraPermissionState.launchPermissionRequest()
                            }
                        }
                    )
                }
                ScanStep.SCANNING -> {
                    if (!cameraPermissionState.status.isGranted) {
                        PermissionRequiredScreen(
                            onRequestPermission = { cameraPermissionState.launchPermissionRequest() }
                        )
                    } else {
                        // 시나리오1: 후면 카메라로 전신 사진 촬영
                        BodyPhotoCaptureScreen(
                            viewModel = viewModel,
                            lifecycleOwner = lifecycleOwner
                        )
                    }
                }
                ScanStep.PREVIEW -> {
                    PreviewScreen(
                        uiState = uiState,
                        onConfirm = { viewModel.createProfile() },
                        onRescan = { viewModel.rescan() }
                    )
                }
                ScanStep.BODY_PHOTO_CAPTURED -> {
                    // 전신 사진 촬영 완료 (시나리오1)
                    LoadingScreen(message = "사진을 처리하고 있습니다...")
                }
                ScanStep.HEIGHT_INPUT -> {
                    // 키 입력 화면 (시나리오1)
                    HeightInputScreen(viewModel = viewModel)
                }
                ScanStep.UPLOADING -> {
                    // 이미지 업로드 중 (시나리오1)
                    LoadingScreen(message = "전신 사진을 업로드하고 있습니다...")
                }
                ScanStep.GENERATING_BODY_DATA -> {
                    // 체형 데이터 생성 중 (시나리오1 - Step 9)
                    LoadingScreen(message = "체형 데이터를 생성하고 있습니다...")
                }
                ScanStep.GENERATING_PROFILE -> {
                    // 맞춤형 차량 프로필 생성 중 (시나리오1 - Step 10)
                    GeneratingProfileScreen()
                }
                ScanStep.CREATING -> {
                    CreatingProfileScreen()
                }
                ScanStep.COMPLETE -> {
                    CompleteScreen(
                        uiState = uiState,
                        onFinish = onScanComplete
                    )
                }
            }

            // 에러 메시지 (사용자 정보 에러는 표시하지 않음)
            if (uiState.errorMessage != null && !uiState.errorMessage!!.contains("사용자 정보")) {
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
 * 안내 화면
 */
@Composable
private fun InstructionScreen(onStartScan: () -> Unit) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(24.dp)
    ) {
        Spacer(modifier = Modifier.weight(1f))

        Icon(
            Icons.Default.Accessibility,
            contentDescription = null,
            modifier = Modifier.size(120.dp),
            tint = MaterialTheme.colorScheme.primary
        )

        Text(
            text = "3D 바디스캔",
            style = MaterialTheme.typography.headlineMedium,
            color = MaterialTheme.colorScheme.primary
        )

        Card {
            Column(
                modifier = Modifier.padding(16.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                Text(
                    text = "스캔 안내",
                    style = MaterialTheme.typography.titleMedium
                )
                InstructionItem("1. 카메라로부터 1~2m 거리를 유지하세요")
                InstructionItem("2. 전신이 화면에 보이도록 서주세요")
                InstructionItem("3. 자연스러운 자세를 유지하세요")
                InstructionItem("4. 스캔이 완료될 때까지 움직이지 마세요")
            }
        }

        Spacer(modifier = Modifier.weight(1f))

        Button(
            onClick = onStartScan,
            modifier = Modifier.fillMaxWidth()
        ) {
            Icon(Icons.Default.CameraAlt, contentDescription = null)
            Spacer(modifier = Modifier.width(8.dp))
            Text("스캔 시작")
        }
    }
}

@Composable
private fun InstructionItem(text: String) {
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
            text = "바디스캔을 위해 카메라 접근 권한이 필요합니다",
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
 * 스캔 중 화면
 */
@Composable
private fun ScanningScreen(
    bodyScanManager: BodyScanManager,
    lifecycleOwner: androidx.lifecycle.LifecycleOwner,
    viewModel: BodyScanViewModel,
    uiState: BodyScanUiState
) {
    val context = LocalContext.current
    val previewView = remember {
        PreviewView(context).apply {
            scaleType = PreviewView.ScaleType.FILL_CENTER
        }
    }

    LaunchedEffect(Unit) {
        bodyScanManager.startBodyScan(
            lifecycleOwner,
            previewView,
            userId = "current_user" // TODO: 실제 userId
        ).collect { result ->
            when (result) {
                is BodyScanManager.BodyScanResult.NoPersonDetected -> {
                    viewModel.onNoPersonDetected()
                }
                is BodyScanManager.BodyScanResult.PersonDetected -> {
                    viewModel.onPersonDetected(result.confidence)
                }
                is BodyScanManager.BodyScanResult.ScanComplete -> {
                    viewModel.onScanComplete(result.bodyScan)
                }
                is BodyScanManager.BodyScanResult.Error -> {
                    viewModel.onScanError(result.message)
                }
                else -> {}
            }
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
                .padding(24.dp),
            verticalArrangement = Arrangement.SpaceBetween
        ) {
            // 상단 상태
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
                        text = "전신이 보이도록 서주세요",
                        style = MaterialTheme.typography.titleMedium
                    )
                    if (uiState.isPersonDetected) {
                        Row(
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.spacedBy(8.dp)
                        ) {
                            Icon(
                                Icons.Default.CheckCircle,
                                contentDescription = null,
                                tint = MaterialTheme.colorScheme.primary
                            )
                            Text(
                                text = "감지됨 (신뢰도: ${(uiState.scanConfidence * 100).toInt()}%)",
                                style = MaterialTheme.typography.bodyMedium,
                                color = MaterialTheme.colorScheme.primary
                            )
                        }
                        if (uiState.scanConfidence > 0.7f) {
                            LinearProgressIndicator(
                                modifier = Modifier.fillMaxWidth(),
                                progress = uiState.scanConfidence
                            )
                            Text(
                                text = "자세를 유지하세요...",
                                style = MaterialTheme.typography.bodySmall
                            )
                        }
                    } else {
                        Text(
                            text = "사람이 감지되지 않았습니다",
                            style = MaterialTheme.typography.bodyMedium,
                            color = MaterialTheme.colorScheme.error
                        )
                    }
                }
            }
        }
    }

    DisposableEffect(Unit) {
        onDispose {
            bodyScanManager.stopBodyScan()
        }
    }
}

/**
 * 스캔 결과 미리보기
 */
@Composable
private fun PreviewScreen(
    uiState: BodyScanUiState,
    onConfirm: () -> Unit,
    onRescan: () -> Unit
) {
    val bodyScan = uiState.bodyScanData ?: return

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(24.dp)
            .verticalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        Icon(
            Icons.Default.CheckCircle,
            contentDescription = null,
            modifier = Modifier
                .size(80.dp)
                .align(Alignment.CenterHorizontally),
            tint = MaterialTheme.colorScheme.primary
        )

        Text(
            text = "스캔 완료!",
            style = MaterialTheme.typography.headlineMedium,
            modifier = Modifier.align(Alignment.CenterHorizontally)
        )

        Text(
            text = "측정된 체형 정보",
            style = MaterialTheme.typography.titleMedium
        )

        Card {
            Column(
                modifier = Modifier.padding(16.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                MeasurementRow("앉은 키", "${bodyScan.sittingHeight.toInt()} cm")
                MeasurementRow("어깨 너비", "${bodyScan.shoulderWidth.toInt()} cm")
                MeasurementRow("팔 길이", "${bodyScan.armLength.toInt()} cm")
                MeasurementRow("머리 높이", "${bodyScan.headHeight.toInt()} cm")
                MeasurementRow("눈 높이", "${bodyScan.eyeHeight.toInt()} cm")
                MeasurementRow("다리 길이", "${bodyScan.legLength.toInt()} cm")
                MeasurementRow("상체 길이", "${bodyScan.torsoLength.toInt()} cm")
            }
        }

        Spacer(modifier = Modifier.weight(1f))

        Button(
            onClick = onConfirm,
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("프로필 생성")
        }

        OutlinedButton(
            onClick = onRescan,
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("다시 스캔")
        }
    }
}

@Composable
private fun MeasurementRow(label: String, value: String) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(
            text = label,
            style = MaterialTheme.typography.bodyLarge,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )
        Text(
            text = value,
            style = MaterialTheme.typography.bodyLarge,
            color = MaterialTheme.colorScheme.primary
        )
    }
}

/**
 * 프로필 생성 중 화면
 */
@Composable
private fun CreatingProfileScreen() {
    Box(
        modifier = Modifier.fillMaxSize(),
        contentAlignment = Alignment.Center
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            CircularProgressIndicator(modifier = Modifier.size(64.dp))
            Text(
                text = "AI가 프로필을 생성 중입니다...",
                style = MaterialTheme.typography.titleMedium
            )
            Text(
                text = "차량 세팅 최적화 계산 중",
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
        }
    }
}

/**
 * 완료 화면
 */
@Composable
private fun CompleteScreen(
    uiState: BodyScanUiState,
    onFinish: () -> Unit
) {
    // 시나리오1: 3초 후 자동으로 메인화면으로 이동
    LaunchedEffect(Unit) {
        kotlinx.coroutines.delay(3000)
        onFinish()
    }

    // 시나리오1: 프로필이 없어도 완료 화면 표시
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(24.dp)
    ) {
        Spacer(modifier = Modifier.weight(1f))

        Icon(
            Icons.Default.Done,
            contentDescription = null,
            modifier = Modifier.size(120.dp),
            tint = MaterialTheme.colorScheme.primary
        )

        Text(
            text = "프로필 생성 완료!",
            style = MaterialTheme.typography.headlineMedium,
            color = MaterialTheme.colorScheme.primary
        )

        Card {
            Column(
                modifier = Modifier.padding(16.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Text(
                    text = "이제 차량이 자동으로 당신에게 맞춰집니다",
                    style = MaterialTheme.typography.bodyLarge
                )
                Text(
                    text = "• 좌석 4축 자동 조절\n• 미러 2축 자동 조절\n• 핸들 2축 자동 조절",
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }

        // 자동 이동 안내
        LinearProgressIndicator(
            modifier = Modifier.fillMaxWidth()
        )
        Text(
            text = "3초 후 메인 화면으로 이동합니다...",
            style = MaterialTheme.typography.bodySmall,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )

        Spacer(modifier = Modifier.weight(1f))

        Button(
            onClick = onFinish,
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("완료")
        }
    }
}

/**
 * 키 입력 화면 (시나리오1)
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun HeightInputScreen(viewModel: BodyScanViewModel) {
    var heightText by remember { mutableStateOf("") }
    var showError by remember { mutableStateOf(false) }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(24.dp)
    ) {
        Spacer(modifier = Modifier.weight(1f))

        Icon(
            Icons.Default.Straighten,
            contentDescription = null,
            modifier = Modifier.size(120.dp),
            tint = MaterialTheme.colorScheme.primary
        )

        Text(
            text = "키를 입력해주세요",
            style = MaterialTheme.typography.headlineMedium,
            color = MaterialTheme.colorScheme.primary
        )

        Text(
            text = "정확한 체형 분석을 위해 키 정보가 필요합니다",
            style = MaterialTheme.typography.bodyLarge,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )

        OutlinedTextField(
            value = heightText,
            onValueChange = {
                heightText = it
                showError = false
            },
            label = { Text("키 (cm)") },
            suffix = { Text("cm") },
            isError = showError,
            keyboardOptions = androidx.compose.foundation.text.KeyboardOptions(
                keyboardType = androidx.compose.ui.text.input.KeyboardType.Number
            ),
            modifier = Modifier.fillMaxWidth()
        )

        if (showError) {
            Text(
                text = "올바른 키를 입력해주세요 (100~250cm)",
                color = MaterialTheme.colorScheme.error,
                style = MaterialTheme.typography.bodySmall
            )
        }

        Spacer(modifier = Modifier.weight(1f))

        Button(
            onClick = {
                val height = heightText.toFloatOrNull()
                if (height != null && height in 100f..250f) {
                    // 시나리오1: 키 입력 후 서버에 업로드
                    viewModel.onHeightConfirmed(height.toInt())
                } else {
                    showError = true
                }
            },
            modifier = Modifier.fillMaxWidth(),
            enabled = heightText.isNotEmpty()
        ) {
            Text("다음")
        }
    }
}

/**
 * 로딩 화면
 */
@Composable
private fun LoadingScreen(message: String) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        CircularProgressIndicator(
            modifier = Modifier.size(64.dp)
        )

        Spacer(modifier = Modifier.height(24.dp))

        Text(
            text = message,
            style = MaterialTheme.typography.bodyLarge,
            color = MaterialTheme.colorScheme.onSurface
        )
    }
}

/**
 * 체형데이터 생성 중 화면 (시나리오1)
 */
@Composable
private fun GeneratingProfileScreen() {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(24.dp)
    ) {
        Spacer(modifier = Modifier.weight(1f))

        // 애니메이션 효과가 있는 프로그레스
        CircularProgressIndicator(
            modifier = Modifier.size(120.dp),
            strokeWidth = 8.dp
        )

        Text(
            text = "맞춤형 차량 세팅이 준비중입니다",
            style = MaterialTheme.typography.headlineMedium,
            color = MaterialTheme.colorScheme.primary
        )

        Card(
            modifier = Modifier.fillMaxWidth()
        ) {
            Column(
                modifier = Modifier.padding(16.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                Text(
                    text = "체형 데이터 분석 중...",
                    style = MaterialTheme.typography.titleMedium
                )

                LinearProgressIndicator(
                    modifier = Modifier.fillMaxWidth()
                )

                Text(
                    text = "• AI가 체형을 분석하고 있습니다\n• 최적의 차량 세팅을 계산하고 있습니다\n• 잠시만 기다려주세요",
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }

        Spacer(modifier = Modifier.weight(1f))
    }
}

/**
 * 전신 사진 촬영 화면 (시나리오1)
 * 후면 카메라로 전신 사진을 촬영
 */
@Composable
private fun BodyPhotoCaptureScreen(
    viewModel: BodyScanViewModel,
    lifecycleOwner: androidx.lifecycle.LifecycleOwner
) {
    val context = LocalContext.current
    var imageCapture by remember { mutableStateOf<ImageCapture?>(null) }
    var isCapturing by remember { mutableStateOf(false) }
    var capturedBitmap by remember { mutableStateOf<Bitmap?>(null) }
    var isFrontCamera by remember { mutableStateOf(false) } // 카메라 전환 상태

    Box(modifier = Modifier.fillMaxSize()) {
        // 카메라 프리뷰
        CameraPreview(
            onImageCaptureReady = { imageCapture = it },
            lifecycleOwner = lifecycleOwner,
            isFrontCamera = isFrontCamera // 상태에 따라 카메라 전환
        )

        // 오버레이 UI
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(24.dp),
            verticalArrangement = Arrangement.SpaceBetween
        ) {
            // 상단 안내 및 카메라 전환
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.Top
            ) {
                Card(
                    modifier = Modifier.weight(1f),
                    colors = CardDefaults.cardColors(
                        containerColor = MaterialTheme.colorScheme.surface.copy(alpha = 0.9f)
                    )
                ) {
                    Column(
                        modifier = Modifier.padding(16.dp),
                        verticalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        Text(
                            text = "전신 촬영",
                            style = MaterialTheme.typography.titleMedium
                        )
                        Text(
                            text = "전신이 화면에 모두 보이도록 서주세요",
                            style = MaterialTheme.typography.bodyMedium,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                    }
                }

                Spacer(modifier = Modifier.width(8.dp))

                // 카메라 전환 버튼
                FilledIconButton(
                    onClick = {
                        isFrontCamera = !isFrontCamera
                        imageCapture = null // 카메라 재초기화 필요
                    },
                    colors = IconButtonDefaults.filledIconButtonColors(
                        containerColor = MaterialTheme.colorScheme.surface.copy(alpha = 0.9f)
                    )
                ) {
                    Icon(
                        Icons.Default.Cameraswitch,
                        contentDescription = "카메라 전환",
                        tint = MaterialTheme.colorScheme.onSurface
                    )
                }
            }

            // 하단 촬영 버튼
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.Center
            ) {
                Button(
                    onClick = {
                        imageCapture?.let { capture ->
                            isCapturing = true
                            takePicture(
                                context = context,
                                imageCapture = capture,
                                onImageCaptured = { bitmap ->
                                    capturedBitmap = bitmap
                                    // HEIGHT_INPUT 단계로 전환
                                    viewModel.onBodyPhotoCaptured(bitmap)
                                },
                                onError = {
                                    isCapturing = false
                                    // 에러 처리
                                }
                            )
                        }
                    },
                    enabled = !isCapturing && imageCapture != null,
                    modifier = Modifier.size(72.dp),
                    shape = CircleShape
                ) {
                    Icon(
                        Icons.Default.CameraAlt,
                        contentDescription = "촬영",
                        modifier = Modifier.size(36.dp)
                    )
                }
            }
        }

        // 로딩 오버레이
        if (isCapturing) {
            Box(
                modifier = Modifier.fillMaxSize(),
                contentAlignment = Alignment.Center
            ) {
                Surface(
                    color = MaterialTheme.colorScheme.surface.copy(alpha = 0.8f),
                    modifier = Modifier.fillMaxSize()
                ) {}
                CircularProgressIndicator()
            }
        }
    }
}

/**
 * 사진 촬영 함수
 */
private fun takePicture(
    context: Context,
    imageCapture: ImageCapture,
    onImageCaptured: (Bitmap) -> Unit,
    onError: (Exception) -> Unit
) {
    val outputFileOptions = ImageCapture.OutputFileOptions.Builder(
        File(context.cacheDir, "body_photo_${System.currentTimeMillis()}.jpg")
    ).build()

    imageCapture.takePicture(
        outputFileOptions,
        ContextCompat.getMainExecutor(context),
        object : ImageCapture.OnImageSavedCallback {
            override fun onImageSaved(outputFileResults: ImageCapture.OutputFileResults) {
                outputFileResults.savedUri?.let { uri ->
                    try {
                        val bitmap = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                            val source = ImageDecoder.createSource(context.contentResolver, uri)
                            ImageDecoder.decodeBitmap(source)
                        } else {
                            @Suppress("DEPRECATION")
                            MediaStore.Images.Media.getBitmap(context.contentResolver, uri)
                        }
                        onImageCaptured(bitmap)
                    } catch (e: Exception) {
                        onError(e)
                    }
                } ?: onError(Exception("Failed to save image"))
            }

            override fun onError(exception: ImageCaptureException) {
                onError(exception)
            }
        }
    )
}

/**
 * CameraPreview Composable
 * 카메라 프리뷰를 보여주고 ImageCapture를 설정
 */
@Composable
private fun CameraPreview(
    onImageCaptureReady: (ImageCapture) -> Unit,
    lifecycleOwner: androidx.lifecycle.LifecycleOwner,
    isFrontCamera: Boolean = false
) {
    val context = LocalContext.current

    AndroidView(
        factory = { ctx ->
            val previewView = PreviewView(ctx).apply {
                scaleType = PreviewView.ScaleType.FILL_CENTER
            }

            val cameraProviderFuture = ProcessCameraProvider.getInstance(ctx)
            cameraProviderFuture.addListener({
                val cameraProvider = cameraProviderFuture.get()

                val preview = Preview.Builder().build().also {
                    it.setSurfaceProvider(previewView.surfaceProvider)
                }

                val imageCapture = ImageCapture.Builder().build()
                onImageCaptureReady(imageCapture)

                val cameraSelector = if (isFrontCamera) {
                    CameraSelector.DEFAULT_FRONT_CAMERA
                } else {
                    CameraSelector.DEFAULT_BACK_CAMERA
                }

                try {
                    cameraProvider.unbindAll()
                    cameraProvider.bindToLifecycle(
                        lifecycleOwner,
                        cameraSelector,
                        preview,
                        imageCapture
                    )
                } catch (exc: Exception) {
                    // Handle error
                }
            }, ContextCompat.getMainExecutor(ctx))

            previewView
        },
        modifier = Modifier.fillMaxSize()
    )
}

