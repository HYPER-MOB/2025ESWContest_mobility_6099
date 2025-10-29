package com.hypermob.mydrive3dx.presentation.control

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import com.hypermob.mydrive3dx.domain.model.VehicleControl

/**
 * Control Screen
 *
 * 차량 제어 화면
 */
@Composable
fun ControlScreen(
    viewModel: ControlViewModel = hiltViewModel()
) {
    val uiState by viewModel.uiState.collectAsState()
    val scrollState = rememberScrollState()

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
            .verticalScroll(scrollState),
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        // 타이틀
        Text(
            text = "차량 제어",
            style = MaterialTheme.typography.headlineMedium,
            color = MaterialTheme.colorScheme.primary
        )

        when {
            uiState.isLoading -> {
                Box(
                    modifier = Modifier.fillMaxWidth(),
                    contentAlignment = Alignment.Center
                ) {
                    CircularProgressIndicator()
                }
            }
            uiState.vehicleControl != null -> {
                VehicleControlContent(
                    vehicleControl = uiState.vehicleControl!!,
                    isExecuting = uiState.isExecutingCommand,
                    onLockDoors = viewModel::lockDoors,
                    onUnlockDoors = viewModel::unlockDoors,
                    onStartEngine = viewModel::startEngine,
                    onStopEngine = viewModel::stopEngine,
                    onTurnOnClimate = viewModel::turnOnClimate,
                    onTurnOffClimate = viewModel::turnOffClimate,
                    onHonkHorn = viewModel::honkHorn,
                    onFlashLights = viewModel::flashLights
                )
            }
            uiState.errorMessage != null -> {
                Text(
                    text = uiState.errorMessage!!,
                    color = MaterialTheme.colorScheme.error
                )
            }
        }

        // 명령 메시지
        if (uiState.commandMessage != null) {
            Card(
                colors = CardDefaults.cardColors(
                    containerColor = MaterialTheme.colorScheme.primaryContainer
                )
            ) {
                Text(
                    text = uiState.commandMessage!!,
                    modifier = Modifier.padding(16.dp),
                    style = MaterialTheme.typography.bodyMedium
                )
            }
        }
    }
}

@Composable
private fun VehicleControlContent(
    vehicleControl: VehicleControl,
    isExecuting: Boolean,
    onLockDoors: () -> Unit,
    onUnlockDoors: () -> Unit,
    onStartEngine: () -> Unit,
    onStopEngine: () -> Unit,
    onTurnOnClimate: () -> Unit,
    onTurnOffClimate: () -> Unit,
    onHonkHorn: () -> Unit,
    onFlashLights: () -> Unit
) {
    // 차량 상태 카드
    VehicleStatusCard(vehicleControl)

    // 문 제어
    ControlSection(
        title = "문 제어",
        icon = Icons.Default.Lock
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Button(
                onClick = onLockDoors,
                modifier = Modifier.weight(1f),
                enabled = !isExecuting
            ) {
                Icon(Icons.Default.Lock, contentDescription = null)
                Spacer(modifier = Modifier.width(4.dp))
                Text("잠금")
            }
            OutlinedButton(
                onClick = onUnlockDoors,
                modifier = Modifier.weight(1f),
                enabled = !isExecuting
            ) {
                Icon(Icons.Default.LockOpen, contentDescription = null)
                Spacer(modifier = Modifier.width(4.dp))
                Text("잠금 해제")
            }
        }
    }

    // 시동 제어
    ControlSection(
        title = "시동 제어",
        icon = Icons.Default.PowerSettingsNew
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Button(
                onClick = onStartEngine,
                modifier = Modifier.weight(1f),
                enabled = !isExecuting && !vehicleControl.engine.isRunning
            ) {
                Icon(Icons.Default.PowerSettingsNew, contentDescription = null)
                Spacer(modifier = Modifier.width(4.dp))
                Text("시동 켜기")
            }
            OutlinedButton(
                onClick = onStopEngine,
                modifier = Modifier.weight(1f),
                enabled = !isExecuting && vehicleControl.engine.isRunning
            ) {
                Icon(Icons.Default.PowerOff, contentDescription = null)
                Spacer(modifier = Modifier.width(4.dp))
                Text("시동 끄기")
            }
        }
    }

    // 에어컨 제어
    ControlSection(
        title = "에어컨 제어",
        icon = Icons.Default.AcUnit
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Button(
                onClick = onTurnOnClimate,
                modifier = Modifier.weight(1f),
                enabled = !isExecuting && !vehicleControl.climate.isOn
            ) {
                Text("켜기 (22°C)")
            }
            OutlinedButton(
                onClick = onTurnOffClimate,
                modifier = Modifier.weight(1f),
                enabled = !isExecuting && vehicleControl.climate.isOn
            ) {
                Text("끄기")
            }
        }
    }

    // 기타 제어
    ControlSection(
        title = "기타",
        icon = Icons.Default.MoreHoriz
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            OutlinedButton(
                onClick = onHonkHorn,
                modifier = Modifier.weight(1f),
                enabled = !isExecuting
            ) {
                Icon(Icons.Default.VolumeUp, contentDescription = null)
                Spacer(modifier = Modifier.width(4.dp))
                Text("경적")
            }
            OutlinedButton(
                onClick = onFlashLights,
                modifier = Modifier.weight(1f),
                enabled = !isExecuting
            ) {
                Icon(Icons.Default.FlashlightOn, contentDescription = null)
                Spacer(modifier = Modifier.width(4.dp))
                Text("라이트")
            }
        }
    }
}

@Composable
private fun VehicleStatusCard(vehicleControl: VehicleControl) {
    Card(modifier = Modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                text = "차량 상태",
                style = MaterialTheme.typography.titleMedium
            )

            Spacer(modifier = Modifier.height(8.dp))

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                StatusItem("배터리", "${vehicleControl.status.batteryLevel}%")
                StatusItem("연료", "${vehicleControl.status.fuelLevel}%")
            }

            Spacer(modifier = Modifier.height(4.dp))

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                StatusItem("시동", if (vehicleControl.engine.isRunning) "켜짐" else "꺼짐")
                StatusItem("에어컨", if (vehicleControl.climate.isOn) "${vehicleControl.climate.temperature}°C" else "꺼짐")
            }
        }
    }
}

@Composable
private fun StatusItem(label: String, value: String) {
    Column {
        Text(
            text = label,
            style = MaterialTheme.typography.bodySmall,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )
        Text(
            text = value,
            style = MaterialTheme.typography.bodyMedium
        )
    }
}

@Composable
private fun ControlSection(
    title: String,
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    content: @Composable () -> Unit
) {
    Card(modifier = Modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(16.dp)) {
            Row(
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Icon(
                    icon,
                    contentDescription = null,
                    tint = MaterialTheme.colorScheme.primary
                )
                Text(
                    text = title,
                    style = MaterialTheme.typography.titleMedium
                )
            }

            Spacer(modifier = Modifier.height(12.dp))

            content()
        }
    }
}
