package com.hypermob.mydrive3dx.presentation.mfa

import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.delay

/**
 * Simple MFA Screen (시나리오3)
 * Polling 방식으로 인증 상태 확인
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SimpleMfaScreen(
    carId: String,
    onAuthSuccess: () -> Unit,
    modifier: Modifier = Modifier
) {
    var nfcCompleted by remember { mutableStateOf(false) }
    var bleCompleted by remember { mutableStateOf(false) }
    var faceCompleted by remember { mutableStateOf(false) }
    var showSuccess by remember { mutableStateOf(false) }
    var pollingCount by remember { mutableStateOf(0) }

    // Polling 방식으로 인증 상태 확인 (1초마다)
    LaunchedEffect(Unit) {
        while (!showSuccess) {
            delay(1000) // 1초마다 폴링
            pollingCount++

            // 시연을 위한 자동 인증 완료 로직
            // 실제로는 서버 API를 호출해서 상태를 확인
            when (pollingCount) {
                3 -> nfcCompleted = true  // 3초 후 NFC 완료
                7 -> bleCompleted = true   // 7초 후 BLE 완료
                12 -> {
                    faceCompleted = true   // 12초 후 얼굴인식 완료
                    showSuccess = true
                }
            }
        }

        // MFA 인증이 모두 완료되었습니다 표시 후 3초 뒤 이동
        delay(3000)
        onAuthSuccess()
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("차량 인수 인증") }
            )
        }
    ) { padding ->
        Column(
            modifier = modifier
                .fillMaxSize()
                .padding(padding)
                .padding(24.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.Center
        ) {
            Text(
                text = "차량 인수를 위한 인증",
                style = MaterialTheme.typography.headlineMedium
            )

            Spacer(modifier = Modifier.height(16.dp))

            Text(
                text = "차량 ID: $carId",
                style = MaterialTheme.typography.bodyLarge,
                color = MaterialTheme.colorScheme.primary
            )

            Spacer(modifier = Modifier.height(48.dp))

            // NFC 인증 카드
            AuthStepCard(
                title = "NFC 태그 인증",
                description = if (nfcCompleted) "인증 완료" else "인증 대기 중...",
                icon = Icons.Default.Nfc,
                isCompleted = nfcCompleted,
                isInProgress = !nfcCompleted
            )

            Spacer(modifier = Modifier.height(16.dp))

            // BLE 인증 카드
            AuthStepCard(
                title = "BLE 연결 인증",
                description = if (bleCompleted) "인증 완료" else if (nfcCompleted) "인증 대기 중..." else "대기",
                icon = Icons.Default.Bluetooth,
                isCompleted = bleCompleted,
                isInProgress = nfcCompleted && !bleCompleted
            )

            Spacer(modifier = Modifier.height(16.dp))

            // 얼굴 인증 카드
            AuthStepCard(
                title = "얼굴 인식",
                description = if (faceCompleted) "인증 완료" else if (bleCompleted) "인증 대기 중..." else "대기",
                icon = Icons.Default.Face,
                isCompleted = faceCompleted,
                isInProgress = bleCompleted && !faceCompleted
            )

            Spacer(modifier = Modifier.height(32.dp))

            // 완료 메시지
            if (showSuccess) {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    colors = CardDefaults.cardColors(
                        containerColor = MaterialTheme.colorScheme.primaryContainer
                    )
                ) {
                    Row(
                        modifier = Modifier.padding(20.dp),
                        horizontalArrangement = Arrangement.spacedBy(16.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Icon(
                            Icons.Default.CheckCircle,
                            contentDescription = null,
                            tint = MaterialTheme.colorScheme.primary,
                            modifier = Modifier.size(40.dp)
                        )
                        Column {
                            Text(
                                text = "MFA 인증이 모두 완료되었습니다",
                                style = MaterialTheme.typography.titleLarge,
                                color = MaterialTheme.colorScheme.primary
                            )
                            Text(
                                text = "3초 후 차량 제어 화면으로 이동합니다",
                                style = MaterialTheme.typography.bodyMedium,
                                color = MaterialTheme.colorScheme.onPrimaryContainer
                            )
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun AuthStepCard(
    title: String,
    description: String,
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    isCompleted: Boolean,
    isInProgress: Boolean
) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = if (isCompleted) {
            CardDefaults.cardColors(
                containerColor = MaterialTheme.colorScheme.primaryContainer
            )
        } else CardDefaults.cardColors()
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(20.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Row(
                horizontalArrangement = Arrangement.spacedBy(16.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Icon(
                    icon,
                    contentDescription = null,
                    modifier = Modifier.size(40.dp),
                    tint = if (isCompleted) {
                        MaterialTheme.colorScheme.primary
                    } else {
                        MaterialTheme.colorScheme.onSurfaceVariant
                    }
                )
                Column {
                    Text(
                        text = title,
                        style = MaterialTheme.typography.titleMedium
                    )
                    Row(
                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        if (isInProgress) {
                            CircularProgressIndicator(
                                modifier = Modifier.size(16.dp),
                                strokeWidth = 2.dp
                            )
                        }
                        Text(
                            text = description,
                            style = MaterialTheme.typography.bodySmall,
                            color = if (isCompleted) {
                                MaterialTheme.colorScheme.primary
                            } else {
                                MaterialTheme.colorScheme.onSurfaceVariant
                            }
                        )
                    }
                }
            }

            if (isCompleted) {
                Icon(
                    Icons.Default.CheckCircle,
                    contentDescription = "완료",
                    tint = MaterialTheme.colorScheme.primary,
                    modifier = Modifier.size(32.dp)
                )
            }
        }
    }
}
