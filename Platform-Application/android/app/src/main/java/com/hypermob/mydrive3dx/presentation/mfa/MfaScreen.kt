package com.hypermob.mydrive3dx.presentation.mfa

import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import com.hypermob.mydrive3dx.data.hardware.FaceDetectionManager

/**
 * MFA Screen
 *
 * MFA 인증 화면 (간단 버전)
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MfaScreen(
    faceDetectionManager: FaceDetectionManager,
    viewModel: MfaViewModel = hiltViewModel(),
    onNavigateBack: () -> Unit,
    onAuthComplete: () -> Unit
) {
    val uiState by viewModel.uiState.collectAsState()

    // 인증 완료 시 화면 이동
    LaunchedEffect(uiState.isComplete) {
        if (uiState.isComplete) {
            onAuthComplete()
        }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("MFA 인증") },
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
                .padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            Text(
                text = "차량 인수를 위해 3단계 인증을 완료해주세요",
                style = MaterialTheme.typography.bodyLarge
            )

            // Face Recognition
            MfaStepCard(
                title = "얼굴 인증",
                description = "카메라로 얼굴을 인식합니다",
                icon = Icons.Default.Face,
                isVerified = uiState.faceVerified,
                isLoading = uiState.isLoading,
                onStartVerification = viewModel::startFaceVerification
            )

            // NFC
            MfaStepCard(
                title = "NFC 인증",
                description = "스마트 키를 태그해주세요",
                icon = Icons.Default.Nfc,
                isVerified = uiState.nfcVerified,
                isLoading = uiState.isLoading,
                enabled = uiState.faceVerified,
                onStartVerification = viewModel::startNfcVerification
            )

            // BLE
            MfaStepCard(
                title = "BLE 인증",
                description = "Bluetooth로 차량과 연결합니다",
                icon = Icons.Default.Bluetooth,
                isVerified = uiState.bleVerified,
                isLoading = uiState.isLoading,
                enabled = uiState.nfcVerified,
                onStartVerification = viewModel::startBleVerification
            )

            // 진행 상황
            if (uiState.allVerified) {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    colors = CardDefaults.cardColors(
                        containerColor = MaterialTheme.colorScheme.primaryContainer
                    )
                ) {
                    Row(
                        modifier = Modifier.padding(16.dp),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        Icon(
                            Icons.Default.CheckCircle,
                            contentDescription = null,
                            tint = MaterialTheme.colorScheme.primary
                        )
                        Text(
                            text = "모든 인증이 완료되었습니다!",
                            style = MaterialTheme.typography.bodyLarge,
                            color = MaterialTheme.colorScheme.onPrimaryContainer
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun MfaStepCard(
    title: String,
    description: String,
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    isVerified: Boolean,
    isLoading: Boolean,
    enabled: Boolean = true,
    onStartVerification: () -> Unit
) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = if (isVerified) {
            CardDefaults.cardColors(
                containerColor = MaterialTheme.colorScheme.primaryContainer
            )
        } else {
            CardDefaults.cardColors()
        }
    ) {
        Column(modifier = Modifier.padding(16.dp)) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Row(
                    horizontalArrangement = Arrangement.spacedBy(12.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Icon(
                        icon,
                        contentDescription = null,
                        modifier = Modifier.size(40.dp),
                        tint = if (isVerified) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.onSurfaceVariant
                    )
                    Column {
                        Text(
                            text = title,
                            style = MaterialTheme.typography.titleMedium
                        )
                        Text(
                            text = description,
                            style = MaterialTheme.typography.bodySmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                    }
                }

                if (isVerified) {
                    Icon(
                        Icons.Default.CheckCircle,
                        contentDescription = "완료",
                        tint = MaterialTheme.colorScheme.primary
                    )
                }
            }

            if (!isVerified && enabled) {
                Spacer(modifier = Modifier.height(12.dp))
                Button(
                    onClick = onStartVerification,
                    modifier = Modifier.fillMaxWidth(),
                    enabled = !isLoading
                ) {
                    if (isLoading) {
                        CircularProgressIndicator(
                            modifier = Modifier.size(20.dp),
                            color = MaterialTheme.colorScheme.onPrimary
                        )
                    } else {
                        Text("인증 시작")
                    }
                }
            }
        }
    }
}
