package com.hypermob.mydrive3dx.presentation.mfa

import androidx.biometric.BiometricPrompt
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.CheckCircle
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import androidx.fragment.app.FragmentActivity
import androidx.hilt.navigation.compose.hiltViewModel
import com.hypermob.mydrive3dx.domain.model.MfaStepStatus

/**
 * MFA Authentication Screen
 * MFA 인증 화면 (BLE + NFC)
 *
 * android_mvp 플로우:
 * - Face Registration은 별도 화면(FaceRegisterScreen)에서 먼저 완료
 * - 이 화면에서는 BLE 연결 + NFC 태그 읽기 + 서버 검증만 수행
 */
@Composable
fun MfaAuthScreen(
    carId: String,  // 차량 ID
    viewModel: MfaAuthViewModel = hiltViewModel(),
    onAuthSuccess: () -> Unit,
    modifier: Modifier = Modifier
) {
    val context = LocalContext.current
    val activity = context as FragmentActivity

    val state by viewModel.state.collectAsState()

    // 인증 성공 시 네비게이션
    LaunchedEffect(state.authSuccess) {
        if (state.authSuccess) {
            onAuthSuccess()
        }
    }

    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        Text(
            text = "Multi-Factor Authentication",
            style = MaterialTheme.typography.headlineMedium
        )

        Spacer(modifier = Modifier.height(32.dp))

        // 1. BLE Authentication (Write "ACCESS")
        MfaStepCard(
            title = "BLE Device Connection",
            status = state.bleStatus,
            onAction = {
                viewModel.scanBleDevice()
            }
        )

        Spacer(modifier = Modifier.height(16.dp))

        // 2. NFC Tag Read (차량 NFC 태그 UID 읽기)
        MfaStepCard(
            title = "NFC Tag Verification",
            status = state.nfcStatus,
            onAction = {
                viewModel.enableNfcReaderMode(activity, carId)
            }
        )

        Spacer(modifier = Modifier.height(32.dp))

        // Loading Indicator
        if (state.isLoading) {
            CircularProgressIndicator()
        }

        // Error Message
        if (state.error != null) {
            Text(
                text = state.error!!,
                color = MaterialTheme.colorScheme.error,
                style = MaterialTheme.typography.bodyMedium
            )
            Spacer(modifier = Modifier.height(8.dp))
            TextButton(onClick = { viewModel.clearError() }) {
                Text("Dismiss")
            }
        }
    }
}

/**
 * MFA Step Card
 * MFA 단계별 카드 UI
 */
@Composable
fun MfaStepCard(
    title: String,
    status: MfaStepStatus,
    onAction: () -> Unit,
    modifier: Modifier = Modifier
) {
    Card(
        modifier = modifier.fillMaxWidth()
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column {
                Text(text = title, style = MaterialTheme.typography.titleMedium)
                Text(
                    text = status.message,
                    style = MaterialTheme.typography.bodySmall,
                    color = when (status) {
                        is MfaStepStatus.Pending -> MaterialTheme.colorScheme.onSurface
                        is MfaStepStatus.InProgress -> MaterialTheme.colorScheme.primary
                        is MfaStepStatus.Completed -> MaterialTheme.colorScheme.primary
                        is MfaStepStatus.Failed -> MaterialTheme.colorScheme.error
                    }
                )
            }

            when (status) {
                is MfaStepStatus.Pending -> {
                    Button(onClick = onAction) {
                        Text("Start")
                    }
                }
                is MfaStepStatus.InProgress -> {
                    CircularProgressIndicator(modifier = Modifier.size(24.dp))
                }
                is MfaStepStatus.Completed -> {
                    Icon(
                        imageVector = Icons.Default.CheckCircle,
                        contentDescription = "Completed",
                        tint = MaterialTheme.colorScheme.primary
                    )
                }
                is MfaStepStatus.Failed -> {
                    TextButton(onClick = onAction) {
                        Text("Retry")
                    }
                }
            }
        }
    }
}

/**
 * Biometric Prompt 표시
 */
private fun showBiometricPrompt(
    activity: FragmentActivity,
    onSuccess: (String) -> Unit
) {
    val executor = ContextCompat.getMainExecutor(activity)

    val biometricPrompt = BiometricPrompt(
        activity,
        executor,
        object : BiometricPrompt.AuthenticationCallback() {
            override fun onAuthenticationSucceeded(result: BiometricPrompt.AuthenticationResult) {
                // 생체 인증 성공 시 토큰 생성
                val token = result.cryptoObject?.cipher?.iv?.toString()
                    ?: "face_token_${System.currentTimeMillis()}"
                onSuccess(token)
            }

            override fun onAuthenticationFailed() {
                // 실패 처리는 ViewModel에서
            }

            override fun onAuthenticationError(errorCode: Int, errString: CharSequence) {
                // 에러 처리는 ViewModel에서
            }
        }
    )

    val promptInfo = BiometricPrompt.PromptInfo.Builder()
        .setTitle("Face Recognition")
        .setSubtitle("Verify your identity")
        .setNegativeButtonText("Cancel")
        .build()

    biometricPrompt.authenticate(promptInfo)
}
