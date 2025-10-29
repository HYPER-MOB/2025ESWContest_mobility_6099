package com.hypermob.mydrive3dx.presentation.vehicle

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import coil.compose.AsyncImage
import com.hypermob.mydrive3dx.domain.model.Vehicle
import com.hypermob.mydrive3dx.domain.model.VehicleStatus

/**
 * Vehicle List Screen
 * 시나리오2: 대여 가능한 차량 목록 표시
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun VehicleListScreen(
    viewModel: VehicleListViewModel = hiltViewModel(),
    onNavigateToDetail: (String) -> Unit,
    onNavigateBack: (() -> Unit)? = null
) {
    val uiState by viewModel.uiState.collectAsState()

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("차량 대여") },
                navigationIcon = if (onNavigateBack != null) { {
                    IconButton(onClick = onNavigateBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "뒤로")
                    }
                } } else { {} },
                actions = {
                    // 정렬 옵션
                    IconButton(onClick = { viewModel.toggleSortOption() }) {
                        Icon(
                            if (uiState.sortByDistance) Icons.Default.LocationOn
                            else Icons.Default.AttachMoney,
                            contentDescription = "정렬"
                        )
                    }
                }
            )
        }
    ) { paddingValues ->
        when {
            uiState.isLoading -> {
                Box(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(paddingValues),
                    contentAlignment = Alignment.Center
                ) {
                    CircularProgressIndicator()
                }
            }
            uiState.vehicles.isEmpty() -> {
                EmptyVehicleList(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(paddingValues)
                )
            }
            else -> {
                LazyColumn(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(paddingValues),
                    contentPadding = PaddingValues(16.dp),
                    verticalArrangement = Arrangement.spacedBy(12.dp)
                ) {
                    // 정렬 정보 표시
                    item {
                        Card(
                            colors = CardDefaults.cardColors(
                                containerColor = MaterialTheme.colorScheme.primaryContainer
                            )
                        ) {
                            Row(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .padding(12.dp),
                                verticalAlignment = Alignment.CenterVertically,
                                horizontalArrangement = Arrangement.spacedBy(8.dp)
                            ) {
                                Icon(
                                    Icons.Default.Info,
                                    contentDescription = null,
                                    modifier = Modifier.size(20.dp)
                                )
                                Text(
                                    text = if (uiState.sortByDistance)
                                        "가까운 순으로 정렬됨"
                                    else
                                        "가격 순으로 정렬됨",
                                    style = MaterialTheme.typography.bodyMedium
                                )
                            }
                        }
                    }

                    // 차량 목록
                    items(uiState.vehicles) { vehicle ->
                        VehicleCard(
                            vehicle = vehicle,
                            onClick = { onNavigateToDetail(vehicle.vehicleId) }
                        )
                    }
                }
            }
        }

        // 에러 메시지
        uiState.errorMessage?.let { error ->
            Snackbar(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(error)
            }
        }
    }
}

@Composable
private fun VehicleCard(
    vehicle: Vehicle,
    onClick: () -> Unit
) {
    Card(
        onClick = onClick,
        modifier = Modifier.fillMaxWidth()
    ) {
        Column {
            // 차량 이미지
            if (!vehicle.imageUrl.isNullOrEmpty()) {
                AsyncImage(
                    model = vehicle.imageUrl,
                    contentDescription = vehicle.model,
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(200.dp),
                    contentScale = ContentScale.Crop
                )
            } else {
                // 플레이스홀더 이미지
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(200.dp),
                    contentAlignment = Alignment.Center
                ) {
                    Icon(
                        Icons.Default.DirectionsCar,
                        contentDescription = null,
                        modifier = Modifier.size(80.dp),
                        tint = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.5f)
                    )
                }
            }

            // 차량 정보
            Column(
                modifier = Modifier.padding(16.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.Top
                ) {
                    // 차량명
                    Column(modifier = Modifier.weight(1f)) {
                        Text(
                            text = "${vehicle.manufacturer} ${vehicle.model}",
                            style = MaterialTheme.typography.titleMedium,
                            fontWeight = FontWeight.Bold
                        )
                        Text(
                            text = vehicle.licensePlate,
                            style = MaterialTheme.typography.bodySmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                    }

                    // 상태 표시
                    StatusChip(status = vehicle.status)
                }

                // 차량 스펙
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(16.dp)
                ) {
                    SpecItem(
                        icon = Icons.Default.LocalGasStation,
                        text = when(vehicle.fuelType) {
                            com.hypermob.mydrive3dx.domain.model.FuelType.ELECTRIC -> "전기"
                            com.hypermob.mydrive3dx.domain.model.FuelType.HYBRID -> "하이브리드"
                            com.hypermob.mydrive3dx.domain.model.FuelType.GASOLINE -> "휘발유"
                            com.hypermob.mydrive3dx.domain.model.FuelType.DIESEL -> "경유"
                            else -> "기타"
                        }
                    )
                    SpecItem(
                        icon = Icons.Default.People,
                        text = "${vehicle.seatingCapacity}인승"
                    )
                    SpecItem(
                        icon = Icons.Default.Speed,
                        text = when(vehicle.transmission) {
                            com.hypermob.mydrive3dx.domain.model.TransmissionType.AUTOMATIC -> "자동"
                            com.hypermob.mydrive3dx.domain.model.TransmissionType.MANUAL -> "수동"
                            else -> "기타"
                        }
                    )
                }

                Divider()

                // 위치 및 가격 정보
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.Bottom
                ) {
                    // 위치 정보
                    Column {
                        Row(
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.spacedBy(4.dp)
                        ) {
                            Icon(
                                Icons.Default.LocationOn,
                                contentDescription = null,
                                modifier = Modifier.size(16.dp),
                                tint = MaterialTheme.colorScheme.onSurfaceVariant
                            )
                            Text(
                                text = vehicle.location,
                                style = MaterialTheme.typography.bodySmall,
                                color = MaterialTheme.colorScheme.onSurfaceVariant
                            )
                        }
                        // 거리 표시
                        vehicle.distance?.let { distance ->
                            Text(
                                text = "%.1f km".format(distance),
                                style = MaterialTheme.typography.labelSmall,
                                color = MaterialTheme.colorScheme.primary
                            )
                        }
                    }

                    // 가격 정보
                    Column(
                        horizontalAlignment = Alignment.End
                    ) {
                        Text(
                            text = "₩${String.format("%,d", vehicle.pricePerHour.toInt())}/시간",
                            style = MaterialTheme.typography.titleSmall,
                            fontWeight = FontWeight.Bold,
                            color = MaterialTheme.colorScheme.primary
                        )
                        Text(
                            text = "₩${String.format("%,d", vehicle.pricePerDay.toInt())}/일",
                            style = MaterialTheme.typography.bodySmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                    }
                }

                // 대여 가능 표시
                if (vehicle.status == VehicleStatus.AVAILABLE) {
                    Button(
                        onClick = onClick,
                        modifier = Modifier.fillMaxWidth()
                    ) {
                        Icon(Icons.Default.DirectionsCar, contentDescription = null)
                        Spacer(modifier = Modifier.width(8.dp))
                        Text("대여하기")
                    }
                }
            }
        }
    }
}

@Composable
private fun StatusChip(status: VehicleStatus) {
    val (text, containerColor, contentColor) = when (status) {
        VehicleStatus.AVAILABLE -> Triple(
            "대여 가능",
            MaterialTheme.colorScheme.primaryContainer,
            MaterialTheme.colorScheme.onPrimaryContainer
        )
        VehicleStatus.RENTED -> Triple(
            "대여 중",
            MaterialTheme.colorScheme.errorContainer,
            MaterialTheme.colorScheme.onErrorContainer
        )
        VehicleStatus.MAINTENANCE -> Triple(
            "정비 중",
            MaterialTheme.colorScheme.tertiaryContainer,
            MaterialTheme.colorScheme.onTertiaryContainer
        )
        VehicleStatus.RESERVED -> Triple(
            "예약됨",
            MaterialTheme.colorScheme.secondaryContainer,
            MaterialTheme.colorScheme.onSecondaryContainer
        )
    }

    Surface(
        color = containerColor,
        shape = MaterialTheme.shapes.small
    ) {
        Text(
            text = text,
            modifier = Modifier.padding(horizontal = 8.dp, vertical = 4.dp),
            style = MaterialTheme.typography.labelSmall,
            color = contentColor
        )
    }
}

@Composable
private fun SpecItem(
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    text: String
) {
    Row(
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(4.dp)
    ) {
        Icon(
            icon,
            contentDescription = null,
            modifier = Modifier.size(16.dp),
            tint = MaterialTheme.colorScheme.onSurfaceVariant
        )
        Text(
            text = text,
            style = MaterialTheme.typography.bodySmall,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )
    }
}

@Composable
private fun EmptyVehicleList(modifier: Modifier = Modifier) {
    Box(
        modifier = modifier,
        contentAlignment = Alignment.Center
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            Icon(
                Icons.Default.DirectionsCar,
                contentDescription = null,
                modifier = Modifier.size(80.dp),
                tint = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.5f)
            )
            Text(
                text = "대여 가능한 차량이 없습니다",
                style = MaterialTheme.typography.bodyLarge,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
        }
    }
}