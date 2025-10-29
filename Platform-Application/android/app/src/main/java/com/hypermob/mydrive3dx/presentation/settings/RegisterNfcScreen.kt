package com.hypermob.mydrive3dx.presentation.settings

import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import com.hypermob.mydrive3dx.data.hardware.NfcManager

/**
 * Register NFC Screen
 *
 * 휴대폰 NFC ID 등록 화면
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun RegisterNfcScreen(
    nfcManager: NfcManager,
    onNavigateBack: () -> Unit,
    onNfcRegistered: (String) -> Unit
) {
    var nfcState by remember { mutableStateOf<NfcRegistrationState>(NfcRegistrationState.Loading) }
    var phoneNfcId by remember { mutableStateOf<String?>(null) }
    var showConfirmDialog by remember { mutableStateOf(false) }

    // 휴대폰 NFC ID 가져오기
    LaunchedEffect(Unit) {
        when {
            !nfcManager.isNfcSupported() -> {
                nfcState = NfcRegistrationState.NotSupported
            }
            !nfcManager.isNfcEnabled() -> {
                nfcState = NfcRegistrationState.Disabled
            }
            else -> {
                // 휴대폰 NFC ID 생성
                phoneNfcId = nfcManager.generatePhoneNfcIdentifier()
                nfcState = NfcRegistrationState.Ready
            }
        }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("휴대폰 NFC 등록") },
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
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.Center
        ) {
            when (nfcState) {
                NfcRegistrationState.Loading -> {
                    CircularProgressIndicator()
                    Spacer(modifier = Modifier.height(16.dp))
                    Text("휴대폰 NFC 정보 확인 중...")
                }

                NfcRegistrationState.NotSupported -> {
                    Icon(
                        Icons.Default.Error,
                        contentDescription = null,
                        modifier = Modifier.size(64.dp),
                        tint = MaterialTheme.colorScheme.error
                    )
                    Spacer(modifier = Modifier.height(16.dp))
                    Text(
                        text = "NFC를 지원하지 않는 기기입니다",
                        style = MaterialTheme.typography.titleMedium,
                        color = MaterialTheme.colorScheme.error
                    )
                    Spacer(modifier = Modifier.height(32.dp))
                    Button(onClick = onNavigateBack) {
                        Text("돌아가기")
                    }
                }

                NfcRegistrationState.Disabled -> {
                    Icon(
                        Icons.Default.Nfc,
                        contentDescription = null,
                        modifier = Modifier.size(64.dp),
                        tint = MaterialTheme.colorScheme.error
                    )
                    Spacer(modifier = Modifier.height(16.dp))
                    Text(
                        text = "NFC가 비활성화되어 있습니다",
                        style = MaterialTheme.typography.titleMedium,
                        color = MaterialTheme.colorScheme.error
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                    Text(
                        text = "설정에서 NFC를 활성화하세요",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                        textAlign = TextAlign.Center
                    )
                    Spacer(modifier = Modifier.height(32.dp))
                    Button(onClick = onNavigateBack) {
                        Text("돌아가기")
                    }
                }

                NfcRegistrationState.Ready -> {
                    Icon(
                        Icons.Default.PhoneAndroid,
                        contentDescription = null,
                        modifier = Modifier.size(120.dp),
                        tint = MaterialTheme.colorScheme.primary
                    )
                    Spacer(modifier = Modifier.height(24.dp))
                    Text(
                        text = "이 휴대폰을 NFC로 등록",
                        style = MaterialTheme.typography.titleLarge,
                        textAlign = TextAlign.Center
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                    Text(
                        text = "차량 인수 시 이 휴대폰을 차량 NFC 리더기에 대면 인증됩니다",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                        textAlign = TextAlign.Center,
                        modifier = Modifier.padding(horizontal = 32.dp)
                    )

                    Spacer(modifier = Modifier.height(32.dp))

                    // NFC ID 정보 카드
                    Card(
                        modifier = Modifier.fillMaxWidth(),
                        colors = CardDefaults.cardColors(
                            containerColor = MaterialTheme.colorScheme.primaryContainer
                        )
                    ) {
                        Column(
                            modifier = Modifier.padding(16.dp),
                            verticalArrangement = Arrangement.spacedBy(8.dp)
                        ) {
                            Text(
                                text = "휴대폰 NFC 정보",
                                style = MaterialTheme.typography.titleMedium,
                                color = MaterialTheme.colorScheme.primary
                            )
                            HorizontalDivider(modifier = Modifier.padding(vertical = 8.dp))

                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.SpaceBetween
                            ) {
                                Text(
                                    text = "NFC ID:",
                                    style = MaterialTheme.typography.bodyMedium,
                                    color = MaterialTheme.colorScheme.onPrimaryContainer
                                )
                                Text(
                                    text = phoneNfcId ?: "-",
                                    style = MaterialTheme.typography.bodySmall,
                                    color = MaterialTheme.colorScheme.onPrimaryContainer
                                )
                            }

                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.SpaceBetween
                            ) {
                                Text(
                                    text = "타입:",
                                    style = MaterialTheme.typography.bodyMedium,
                                    color = MaterialTheme.colorScheme.onPrimaryContainer
                                )
                                Text(
                                    text = "휴대폰 NFC",
                                    style = MaterialTheme.typography.bodyMedium,
                                    color = MaterialTheme.colorScheme.onPrimaryContainer
                                )
                            }
                        }
                    }

                    Spacer(modifier = Modifier.height(32.dp))

                    // 등록 버튼
                    Button(
                        onClick = { showConfirmDialog = true },
                        modifier = Modifier.fillMaxWidth()
                    ) {
                        Icon(Icons.Default.Save, contentDescription = null)
                        Spacer(modifier = Modifier.width(8.dp))
                        Text("이 휴대폰 NFC 등록")
                    }

                    Spacer(modifier = Modifier.height(24.dp))

                    // 안내 카드
                    Card(
                        modifier = Modifier.fillMaxWidth(),
                        colors = CardDefaults.cardColors(
                            containerColor = MaterialTheme.colorScheme.surfaceVariant
                        )
                    ) {
                        Row(
                            modifier = Modifier.padding(16.dp),
                            horizontalArrangement = Arrangement.spacedBy(12.dp)
                        ) {
                            Icon(
                                Icons.Default.Info,
                                contentDescription = null,
                                tint = MaterialTheme.colorScheme.onSurfaceVariant
                            )
                            Column {
                                Text(
                                    text = "NFC 등록 안내",
                                    style = MaterialTheme.typography.titleSmall,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant
                                )
                                Spacer(modifier = Modifier.height(4.dp))
                                Text(
                                    text = "• 이 휴대폰의 고유 NFC ID가 서버에 등록됩니다\n" +
                                            "• 차량 인수 시 휴대폰을 NFC 리더기에 접촉하세요\n" +
                                            "• 등록은 한 번만 하면 됩니다",
                                    style = MaterialTheme.typography.bodySmall,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant
                                )
                            }
                        }
                    }
                }

                NfcRegistrationState.Registering -> {
                    CircularProgressIndicator()
                    Spacer(modifier = Modifier.height(16.dp))
                    Text("NFC ID 등록 중...")
                }

                is NfcRegistrationState.Success -> {
                    Icon(
                        Icons.Default.CheckCircle,
                        contentDescription = null,
                        modifier = Modifier.size(120.dp),
                        tint = MaterialTheme.colorScheme.primary
                    )
                    Spacer(modifier = Modifier.height(24.dp))
                    Text(
                        text = "등록 완료!",
                        style = MaterialTheme.typography.titleLarge,
                        color = MaterialTheme.colorScheme.primary
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                    Text(
                        text = "휴대폰 NFC가 성공적으로 등록되었습니다",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                        textAlign = TextAlign.Center
                    )
                    Spacer(modifier = Modifier.height(32.dp))
                    Button(onClick = onNavigateBack) {
                        Text("완료")
                    }
                }

                is NfcRegistrationState.Error -> {
                    Icon(
                        Icons.Default.Error,
                        contentDescription = null,
                        modifier = Modifier.size(64.dp),
                        tint = MaterialTheme.colorScheme.error
                    )
                    Spacer(modifier = Modifier.height(16.dp))
                    Text(
                        text = "등록 실패",
                        style = MaterialTheme.typography.titleMedium,
                        color = MaterialTheme.colorScheme.error
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                    Text(
                        text = (nfcState as NfcRegistrationState.Error).message,
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                        textAlign = TextAlign.Center
                    )
                    Spacer(modifier = Modifier.height(32.dp))
                    Button(onClick = { nfcState = NfcRegistrationState.Ready }) {
                        Text("다시 시도")
                    }
                }
            }
        }
    }

    // 등록 확인 다이얼로그
    if (showConfirmDialog) {
        AlertDialog(
            onDismissRequest = { showConfirmDialog = false },
            icon = {
                Icon(Icons.Default.Nfc, contentDescription = null)
            },
            title = {
                Text("휴대폰 NFC 등록")
            },
            text = {
                Column {
                    Text("이 휴대폰을 차량 인증용 NFC로 등록하시겠습니까?")
                    Spacer(modifier = Modifier.height(16.dp))
                    Text(
                        text = "NFC ID: ${phoneNfcId?.take(16)}...",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            },
            confirmButton = {
                Button(
                    onClick = {
                        showConfirmDialog = false
                        nfcState = NfcRegistrationState.Registering

                        phoneNfcId?.let { nfcId ->
                            // TODO: 서버에 NFC ID 등록
                            // 현재는 콜백만 호출
                            onNfcRegistered(nfcId)
                            nfcState = NfcRegistrationState.Success(nfcId)
                        } ?: run {
                            nfcState = NfcRegistrationState.Error("NFC ID를 가져올 수 없습니다")
                        }
                    }
                ) {
                    Text("등록")
                }
            },
            dismissButton = {
                TextButton(onClick = { showConfirmDialog = false }) {
                    Text("취소")
                }
            }
        )
    }
}

sealed class NfcRegistrationState {
    object Loading : NfcRegistrationState()
    object NotSupported : NfcRegistrationState()
    object Disabled : NfcRegistrationState()
    object Ready : NfcRegistrationState()
    object Registering : NfcRegistrationState()
    data class Success(val nfcId: String) : NfcRegistrationState()
    data class Error(val message: String) : NfcRegistrationState()
}
