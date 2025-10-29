package com.hypermob.mydrive3dx.presentation.mfa

import android.app.Activity
import android.nfc.Tag
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.CheckCircle
import androidx.compose.material.icons.filled.Nfc
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import com.hypermob.mydrive3dx.data.hardware.NfcManager

/**
 * NFC Verification Screen
 *
 * Android NFC API를 사용한 실제 NFC 인증 화면
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun NfcVerificationScreen(
    viewModel: MfaViewModel = hiltViewModel(),
    nfcManager: NfcManager,
    onVerificationComplete: () -> Unit,
    onNavigateBack: () -> Unit
) {
    val context = LocalContext.current
    val activity = context as? Activity

    var nfcResult by remember { mutableStateOf<NfcManager.NfcResult>(NfcManager.NfcResult.Idle) }
    var isVerified by remember { mutableStateOf(false) }
    var isScanning by remember { mutableStateOf(false) }

    // NFC 태그 감지 콜백
    val onTagDetected: (Tag) -> Unit = { tag ->
        val detected = nfcManager.parseTag(tag)
        nfcResult = detected

        // 차량 키 검증
        val rentalId = viewModel.uiState.value.rentalId ?: ""
        val isValid = nfcManager.verifyVehicleKey(detected.tagId, rentalId)

        if (isValid && !isVerified) {
            isVerified = true
            viewModel.setNfcVerified()
        }
    }

    // NFC Reader Mode 활성화/비활성화
    LaunchedEffect(isScanning) {
        if (isScanning && activity != null) {
            nfcManager.enableReaderMode(activity, onTagDetected)
        }
    }

    DisposableEffect(Unit) {
        onDispose {
            if (activity != null) {
                nfcManager.disableReaderMode(activity)
            }
        }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("NFC 인증") },
                navigationIcon = {
                    IconButton(onClick = {
                        if (activity != null) {
                            nfcManager.disableReaderMode(activity)
                        }
                        onNavigateBack()
                    }) {
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
                .padding(24.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(24.dp)
        ) {
            Spacer(modifier = Modifier.weight(1f))

            // NFC 아이콘
            Icon(
                Icons.Default.Nfc,
                contentDescription = null,
                modifier = Modifier.size(120.dp),
                tint = if (isVerified) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.onSurfaceVariant
            )

            // 상태 메시지
            when {
                !nfcManager.isNfcSupported() -> {
                    Card(
                        colors = CardDefaults.cardColors(
                            containerColor = MaterialTheme.colorScheme.errorContainer
                        )
                    ) {
                        Column(
                            modifier = Modifier.padding(16.dp),
                            horizontalAlignment = Alignment.CenterHorizontally
                        ) {
                            Text(
                                text = "NFC를 지원하지 않는 기기입니다",
                                style = MaterialTheme.typography.titleMedium,
                                color = MaterialTheme.colorScheme.onErrorContainer
                            )
                        }
                    }
                }
                !nfcManager.isNfcEnabled() -> {
                    Card(
                        colors = CardDefaults.cardColors(
                            containerColor = MaterialTheme.colorScheme.tertiaryContainer
                        )
                    ) {
                        Column(
                            modifier = Modifier.padding(16.dp),
                            horizontalAlignment = Alignment.CenterHorizontally
                        ) {
                            Text(
                                text = "NFC가 비활성화되어 있습니다",
                                style = MaterialTheme.typography.titleMedium,
                                color = MaterialTheme.colorScheme.onTertiaryContainer
                            )
                            Spacer(modifier = Modifier.height(8.dp))
                            Text(
                                text = "설정에서 NFC를 활성화해주세요",
                                style = MaterialTheme.typography.bodyMedium,
                                color = MaterialTheme.colorScheme.onTertiaryContainer
                            )
                        }
                    }
                }
                isVerified -> {
                    Card(
                        colors = CardDefaults.cardColors(
                            containerColor = MaterialTheme.colorScheme.primaryContainer
                        )
                    ) {
                        Row(
                            modifier = Modifier.padding(16.dp),
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.spacedBy(12.dp)
                        ) {
                            Icon(
                                Icons.Default.CheckCircle,
                                contentDescription = null,
                                tint = MaterialTheme.colorScheme.primary,
                                modifier = Modifier.size(32.dp)
                            )
                            Column {
                                Text(
                                    text = "NFC 인증 완료!",
                                    style = MaterialTheme.typography.titleLarge,
                                    color = MaterialTheme.colorScheme.onPrimaryContainer
                                )
                                when (val result = nfcResult) {
                                    is NfcManager.NfcResult.TagDetected -> {
                                        Text(
                                            text = "태그 ID: ${result.tagId}",
                                            style = MaterialTheme.typography.bodySmall,
                                            color = MaterialTheme.colorScheme.onPrimaryContainer
                                        )
                                    }
                                    else -> {}
                                }
                            }
                        }
                    }
                }
                isScanning -> {
                    Column(
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.spacedBy(16.dp)
                    ) {
                        CircularProgressIndicator()
                        Text(
                            text = "스마트 키를 기기에 가까이 대주세요",
                            style = MaterialTheme.typography.titleMedium
                        )
                        when (val result = nfcResult) {
                            is NfcManager.NfcResult.TagDetected -> {
                                Card {
                                    Column(
                                        modifier = Modifier.padding(16.dp),
                                        verticalArrangement = Arrangement.spacedBy(4.dp)
                                    ) {
                                        Text(
                                            text = "태그 감지됨",
                                            style = MaterialTheme.typography.bodyLarge
                                        )
                                        Text(
                                            text = "ID: ${result.tagId}",
                                            style = MaterialTheme.typography.bodySmall
                                        )
                                        Text(
                                            text = "타입: ${result.tagType}",
                                            style = MaterialTheme.typography.bodySmall
                                        )
                                    }
                                }
                            }
                            is NfcManager.NfcResult.Error -> {
                                Text(
                                    text = "오류: ${result.message}",
                                    style = MaterialTheme.typography.bodyMedium,
                                    color = MaterialTheme.colorScheme.error
                                )
                            }
                            else -> {}
                        }
                    }
                }
                else -> {
                    Text(
                        text = "스마트 키를 태그하여 인증을 시작하세요",
                        style = MaterialTheme.typography.titleMedium
                    )
                }
            }

            Spacer(modifier = Modifier.weight(1f))

            // 버튼
            if (nfcManager.isNfcSupported() && nfcManager.isNfcEnabled()) {
                if (isVerified) {
                    Button(
                        onClick = onVerificationComplete,
                        modifier = Modifier.fillMaxWidth()
                    ) {
                        Text("다음 단계로")
                    }
                } else {
                    Button(
                        onClick = {
                            isScanning = true
                            nfcResult = NfcManager.NfcResult.Scanning
                        },
                        modifier = Modifier.fillMaxWidth(),
                        enabled = !isScanning
                    ) {
                        Text(if (isScanning) "스캔 중..." else "스캔 시작")
                    }
                }
            }
        }
    }
}
