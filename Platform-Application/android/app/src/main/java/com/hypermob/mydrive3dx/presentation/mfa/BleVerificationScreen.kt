package com.hypermob.mydrive3dx.presentation.mfa

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.Bluetooth
import androidx.compose.material.icons.filled.CheckCircle
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import com.google.accompanist.permissions.ExperimentalPermissionsApi
import com.google.accompanist.permissions.rememberMultiplePermissionsState
import com.hypermob.mydrive3dx.data.hardware.BleManager

/**
 * BLE Verification Screen
 *
 * Android Bluetooth LE API를 사용한 실제 BLE 인증 화면
 */
@OptIn(ExperimentalMaterial3Api::class, ExperimentalPermissionsApi::class)
@Composable
fun BleVerificationScreen(
    viewModel: MfaViewModel = hiltViewModel(),
    bleManager: BleManager,
    onVerificationComplete: () -> Unit,
    onNavigateBack: () -> Unit
) {
    // Bluetooth 권한
    val bluetoothPermissions = if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.S) {
        rememberMultiplePermissionsState(
            listOf(
                android.Manifest.permission.BLUETOOTH_SCAN,
                android.Manifest.permission.BLUETOOTH_CONNECT
            )
        )
    } else {
        rememberMultiplePermissionsState(
            listOf(
                android.Manifest.permission.ACCESS_FINE_LOCATION
            )
        )
    }

    var bleResult by remember { mutableStateOf<BleManager.BleResult>(BleManager.BleResult.Idle) }
    var isVerified by remember { mutableStateOf(false) }
    var isScanning by remember { mutableStateOf(false) }
    var foundDevices by remember { mutableStateOf<List<BleManager.BleResult.DeviceFound>>(emptyList()) }

    // BLE 스캔 시작
    LaunchedEffect(isScanning) {
        if (isScanning && bluetoothPermissions.allPermissionsGranted) {
            bleManager.startScan().collect { result ->
                bleResult = result

                when (result) {
                    is BleManager.BleResult.DeviceFound -> {
                        // 장치 목록에 추가 (중복 제거)
                        if (!foundDevices.any { it.address == result.address }) {
                            foundDevices = foundDevices + result
                        }

                        // 차량 장치를 찾으면 자동 인증
                        if (result.isVehicle && !isVerified) {
                            val rentalId = viewModel.uiState.value.rentalId ?: ""
                            val isValid = bleManager.verifyVehicleConnection(result.address, rentalId)

                            if (isValid) {
                                isVerified = true
                                viewModel.setBleVerified()
                                isScanning = false
                            }
                        }
                    }
                    else -> {}
                }
            }
        }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("BLE 인증") },
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
            // 상태 표시
            when {
                !bleManager.isBluetoothSupported() -> {
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
                                text = "Bluetooth를 지원하지 않는 기기입니다",
                                style = MaterialTheme.typography.titleMedium,
                                color = MaterialTheme.colorScheme.onErrorContainer
                            )
                        }
                    }
                }
                !bleManager.isBluetoothEnabled() -> {
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
                                text = "Bluetooth가 비활성화되어 있습니다",
                                style = MaterialTheme.typography.titleMedium,
                                color = MaterialTheme.colorScheme.onTertiaryContainer
                            )
                            Spacer(modifier = Modifier.height(8.dp))
                            Text(
                                text = "설정에서 Bluetooth를 활성화해주세요",
                                style = MaterialTheme.typography.bodyMedium,
                                color = MaterialTheme.colorScheme.onTertiaryContainer
                            )
                        }
                    }
                }
                !bluetoothPermissions.allPermissionsGranted -> {
                    Card {
                        Column(
                            modifier = Modifier.padding(16.dp),
                            horizontalAlignment = Alignment.CenterHorizontally,
                            verticalArrangement = Arrangement.spacedBy(8.dp)
                        ) {
                            Text(
                                text = "BLE 스캔을 위해 권한이 필요합니다",
                                style = MaterialTheme.typography.titleMedium
                            )
                            Button(onClick = { bluetoothPermissions.launchMultiplePermissionRequest() }) {
                                Text("권한 허용")
                            }
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
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(16.dp),
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
                                    text = "BLE 인증 완료!",
                                    style = MaterialTheme.typography.titleLarge,
                                    color = MaterialTheme.colorScheme.onPrimaryContainer
                                )
                                val vehicleDevice = foundDevices.firstOrNull { it.isVehicle }
                                if (vehicleDevice != null) {
                                    Text(
                                        text = "차량: ${vehicleDevice.name ?: "Unknown"}",
                                        style = MaterialTheme.typography.bodySmall,
                                        color = MaterialTheme.colorScheme.onPrimaryContainer
                                    )
                                }
                            }
                        }
                    }
                }
                else -> {
                    Card {
                        Column(
                            modifier = Modifier.padding(16.dp),
                            horizontalAlignment = Alignment.CenterHorizontally,
                            verticalArrangement = Arrangement.spacedBy(8.dp)
                        ) {
                            Icon(
                                Icons.Default.Bluetooth,
                                contentDescription = null,
                                modifier = Modifier.size(64.dp),
                                tint = MaterialTheme.colorScheme.primary
                            )
                            Text(
                                text = "차량과 BLE로 연결합니다",
                                style = MaterialTheme.typography.titleMedium
                            )
                            if (isScanning) {
                                CircularProgressIndicator()
                                Text(
                                    text = "차량을 검색 중...",
                                    style = MaterialTheme.typography.bodyMedium
                                )
                            }
                        }
                    }
                }
            }

            // 발견된 장치 목록
            if (foundDevices.isNotEmpty() && !isVerified) {
                Text(
                    text = "발견된 장치 (${foundDevices.size}개)",
                    style = MaterialTheme.typography.titleSmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )

                LazyColumn(
                    modifier = Modifier.weight(1f),
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    items(foundDevices) { device ->
                        Card(
                            colors = if (device.isVehicle) {
                                CardDefaults.cardColors(
                                    containerColor = MaterialTheme.colorScheme.primaryContainer
                                )
                            } else {
                                CardDefaults.cardColors()
                            }
                        ) {
                            Row(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .padding(12.dp),
                                horizontalArrangement = Arrangement.SpaceBetween,
                                verticalAlignment = Alignment.CenterVertically
                            ) {
                                Column {
                                    Text(
                                        text = device.name ?: "Unknown Device",
                                        style = MaterialTheme.typography.bodyLarge
                                    )
                                    Text(
                                        text = device.address,
                                        style = MaterialTheme.typography.bodySmall,
                                        color = MaterialTheme.colorScheme.onSurfaceVariant
                                    )
                                    if (device.isVehicle) {
                                        Text(
                                            text = "차량 장치",
                                            style = MaterialTheme.typography.bodySmall,
                                            color = MaterialTheme.colorScheme.primary
                                        )
                                    }
                                }
                                Text(
                                    text = "${device.rssi} dBm",
                                    style = MaterialTheme.typography.bodySmall,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant
                                )
                            }
                        }
                    }
                }
            }

            // 버튼
            if (bleManager.isBluetoothSupported() &&
                bleManager.isBluetoothEnabled() &&
                bluetoothPermissions.allPermissionsGranted) {
                if (isVerified) {
                    Button(
                        onClick = onVerificationComplete,
                        modifier = Modifier.fillMaxWidth()
                    ) {
                        Text("인증 완료")
                    }
                } else {
                    Button(
                        onClick = {
                            if (isScanning) {
                                isScanning = false
                                foundDevices = emptyList()
                            } else {
                                isScanning = true
                            }
                        },
                        modifier = Modifier.fillMaxWidth()
                    ) {
                        Text(if (isScanning) "스캔 중지" else "스캔 시작")
                    }
                }
            }
        }
    }
}
