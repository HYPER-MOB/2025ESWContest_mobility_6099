package com.hypermob.mydrive3dx.presentation.vehicle

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
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
import java.time.LocalDateTime
import java.time.format.DateTimeFormatter

/**
 * Vehicle Detail Screen
 * 시나리오2: 차량 상세 정보 및 대여하기
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun VehicleDetailScreen(
    vehicleId: String,
    viewModel: VehicleDetailViewModel = hiltViewModel(),
    onNavigateBack: () -> Unit,
    onNavigateToApproval: (String) -> Unit
) {
    val uiState by viewModel.uiState.collectAsState()

    // 차량 정보 로드
    LaunchedEffect(vehicleId) {
        viewModel.loadVehicle(vehicleId)
    }

    // 대여 성공 시 승인 화면으로 이동
    LaunchedEffect(uiState.rentalId) {
        uiState.rentalId?.let { rentalId ->
            onNavigateToApproval(rentalId)
        }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("차량 상세") },
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "뒤로")
                    }
                },
                actions = {
                    IconButton(onClick = { /* 공유 */ }) {
                        Icon(Icons.Default.Share, contentDescription = "공유")
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
            uiState.vehicle != null -> {
                val vehicle = uiState.vehicle
                if (vehicle != null) {
                    VehicleDetailContent(
                        vehicle = vehicle,
                        rentalState = uiState.rentalState,
                        onRequestRental = { startTime, endTime ->
                            viewModel.requestRental(startTime, endTime)
                        },
                        modifier = Modifier
                            .fillMaxSize()
                            .padding(paddingValues)
                    )
                }
            }
            else -> {
                Box(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(paddingValues),
                    contentAlignment = Alignment.Center
                ) {
                    Text("차량 정보를 불러올 수 없습니다")
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

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun VehicleDetailContent(
    vehicle: Vehicle,
    rentalState: RentalState,
    onRequestRental: (String, String) -> Unit,
    modifier: Modifier = Modifier
) {
    var showDatePicker by remember { mutableStateOf(false) }
    var selectedStartTime by remember { mutableStateOf(LocalDateTime.now().plusHours(1)) }
    var selectedEndTime by remember { mutableStateOf(LocalDateTime.now().plusHours(4)) }

    Column(
        modifier = modifier
            .verticalScroll(rememberScrollState())
    ) {
        // 차량 이미지
        if (!vehicle.imageUrl.isNullOrEmpty()) {
            AsyncImage(
                model = vehicle.imageUrl,
                contentDescription = vehicle.model,
                modifier = Modifier
                    .fillMaxWidth()
                    .height(250.dp),
                contentScale = ContentScale.Crop
            )
        } else {
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(250.dp),
                contentAlignment = Alignment.Center
            ) {
                Icon(
                    Icons.Default.DirectionsCar,
                    contentDescription = null,
                    modifier = Modifier.size(100.dp),
                    tint = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.5f)
                )
            }
        }

        Column(
            modifier = Modifier.padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            // 차량명 및 상태
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.Top
            ) {
                Column(modifier = Modifier.weight(1f)) {
                    Text(
                        text = "${vehicle.manufacturer} ${vehicle.model}",
                        style = MaterialTheme.typography.headlineMedium,
                        fontWeight = FontWeight.Bold
                    )
                    Text(
                        text = "${vehicle.year}년식 · ${vehicle.color}",
                        style = MaterialTheme.typography.bodyLarge,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                    Text(
                        text = vehicle.licensePlate,
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }

                // 상태 표시
                if (vehicle.status == VehicleStatus.AVAILABLE) {
                    Surface(
                        color = MaterialTheme.colorScheme.primaryContainer,
                        shape = MaterialTheme.shapes.small
                    ) {
                        Text(
                            text = "대여 가능",
                            modifier = Modifier.padding(horizontal = 12.dp, vertical = 6.dp),
                            style = MaterialTheme.typography.labelLarge,
                            color = MaterialTheme.colorScheme.onPrimaryContainer
                        )
                    }
                }
            }

            Divider()

            // 차량 스펙
            Card {
                Column(
                    modifier = Modifier.padding(16.dp),
                    verticalArrangement = Arrangement.spacedBy(12.dp)
                ) {
                    Text(
                        text = "차량 정보",
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.Bold
                    )

                    SpecRow(
                        icon = Icons.Default.LocalGasStation,
                        label = "연료",
                        value = when(vehicle.fuelType) {
                            com.hypermob.mydrive3dx.domain.model.FuelType.ELECTRIC -> "전기"
                            com.hypermob.mydrive3dx.domain.model.FuelType.HYBRID -> "하이브리드"
                            com.hypermob.mydrive3dx.domain.model.FuelType.GASOLINE -> "휘발유"
                            com.hypermob.mydrive3dx.domain.model.FuelType.DIESEL -> "경유"
                            else -> "기타"
                        }
                    )

                    SpecRow(
                        icon = Icons.Default.Speed,
                        label = "변속기",
                        value = when(vehicle.transmission) {
                            com.hypermob.mydrive3dx.domain.model.TransmissionType.AUTOMATIC -> "자동"
                            com.hypermob.mydrive3dx.domain.model.TransmissionType.MANUAL -> "수동"
                            else -> "기타"
                        }
                    )

                    SpecRow(
                        icon = Icons.Default.People,
                        label = "정원",
                        value = "${vehicle.seatingCapacity}인승"
                    )

                    SpecRow(
                        icon = Icons.Default.DirectionsCar,
                        label = "차종",
                        value = when(vehicle.vehicleType) {
                            com.hypermob.mydrive3dx.domain.model.VehicleType.SEDAN -> "세단"
                            com.hypermob.mydrive3dx.domain.model.VehicleType.SUV -> "SUV"
                            com.hypermob.mydrive3dx.domain.model.VehicleType.ELECTRIC -> "전기차"
                            else -> "기타"
                        }
                    )
                }
            }

            // 특징
            if (vehicle.features.isNotEmpty()) {
                Card {
                    Column(
                        modifier = Modifier.padding(16.dp),
                        verticalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        Text(
                            text = "주요 특징",
                            style = MaterialTheme.typography.titleMedium,
                            fontWeight = FontWeight.Bold
                        )
                        vehicle.features.forEach { feature ->
                            Row(
                                verticalAlignment = Alignment.CenterVertically,
                                horizontalArrangement = Arrangement.spacedBy(8.dp)
                            ) {
                                Icon(
                                    Icons.Default.Check,
                                    contentDescription = null,
                                    modifier = Modifier.size(20.dp),
                                    tint = MaterialTheme.colorScheme.primary
                                )
                                Text(
                                    text = feature,
                                    style = MaterialTheme.typography.bodyMedium
                                )
                            }
                        }
                    }
                }
            }

            // 위치 정보
            Card {
                Column(
                    modifier = Modifier.padding(16.dp),
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    Row(
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        Icon(
                            Icons.Default.LocationOn,
                            contentDescription = null,
                            tint = MaterialTheme.colorScheme.primary
                        )
                        Text(
                            text = "픽업 위치",
                            style = MaterialTheme.typography.titleMedium,
                            fontWeight = FontWeight.Bold
                        )
                    }
                    Text(
                        text = vehicle.location,
                        style = MaterialTheme.typography.bodyLarge
                    )
                }
            }

            // 가격 정보
            Card(
                colors = CardDefaults.cardColors(
                    containerColor = MaterialTheme.colorScheme.primaryContainer
                )
            ) {
                Column(
                    modifier = Modifier.padding(16.dp),
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    Text(
                        text = "대여 요금",
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.Bold
                    )
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Column {
                            Text(
                                text = "시간당",
                                style = MaterialTheme.typography.bodyMedium,
                                color = MaterialTheme.colorScheme.onPrimaryContainer.copy(alpha = 0.7f)
                            )
                            Text(
                                text = "₩${String.format("%,d", vehicle.pricePerHour.toInt())}",
                                style = MaterialTheme.typography.titleLarge,
                                fontWeight = FontWeight.Bold,
                                color = MaterialTheme.colorScheme.onPrimaryContainer
                            )
                        }
                        Column(horizontalAlignment = Alignment.End) {
                            Text(
                                text = "일일",
                                style = MaterialTheme.typography.bodyMedium,
                                color = MaterialTheme.colorScheme.onPrimaryContainer.copy(alpha = 0.7f)
                            )
                            Text(
                                text = "₩${String.format("%,d", vehicle.pricePerDay.toInt())}",
                                style = MaterialTheme.typography.titleLarge,
                                fontWeight = FontWeight.Bold,
                                color = MaterialTheme.colorScheme.onPrimaryContainer
                            )
                        }
                    }
                }
            }

            // 대여 시간 선택
            Card {
                Column(
                    modifier = Modifier.padding(16.dp),
                    verticalArrangement = Arrangement.spacedBy(12.dp)
                ) {
                    Text(
                        text = "대여 기간",
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.Bold
                    )

                    OutlinedTextField(
                        value = selectedStartTime.format(DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm")),
                        onValueChange = {},
                        label = { Text("시작 시간") },
                        readOnly = true,
                        modifier = Modifier.fillMaxWidth(),
                        trailingIcon = {
                            IconButton(onClick = { showDatePicker = true }) {
                                Icon(Icons.Default.Schedule, contentDescription = "시간 선택")
                            }
                        }
                    )

                    OutlinedTextField(
                        value = selectedEndTime.format(DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm")),
                        onValueChange = {},
                        label = { Text("종료 시간") },
                        readOnly = true,
                        modifier = Modifier.fillMaxWidth(),
                        trailingIcon = {
                            IconButton(onClick = { showDatePicker = true }) {
                                Icon(Icons.Default.Schedule, contentDescription = "시간 선택")
                            }
                        }
                    )

                    // 예상 요금
                    val hours = java.time.Duration.between(selectedStartTime, selectedEndTime).toHours()
                    val estimatedPrice = if (hours >= 24) {
                        (hours / 24 * vehicle.pricePerDay).toInt()
                    } else {
                        (hours * vehicle.pricePerHour).toInt()
                    }

                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Text(
                            text = "예상 요금",
                            style = MaterialTheme.typography.bodyLarge
                        )
                        Text(
                            text = "₩${String.format("%,d", estimatedPrice)}",
                            style = MaterialTheme.typography.titleMedium,
                            fontWeight = FontWeight.Bold,
                            color = MaterialTheme.colorScheme.primary
                        )
                    }
                }
            }

            // 대여하기 버튼
            if (vehicle.status == VehicleStatus.AVAILABLE) {
                Button(
                    onClick = {
                        onRequestRental(
                            selectedStartTime.format(DateTimeFormatter.ISO_LOCAL_DATE_TIME),
                            selectedEndTime.format(DateTimeFormatter.ISO_LOCAL_DATE_TIME)
                        )
                    },
                    modifier = Modifier.fillMaxWidth(),
                    enabled = rentalState != RentalState.REQUESTING
                ) {
                    when (rentalState) {
                        RentalState.REQUESTING -> {
                            CircularProgressIndicator(
                                modifier = Modifier.size(20.dp),
                                color = MaterialTheme.colorScheme.onPrimary
                            )
                        }
                        else -> {
                            Icon(Icons.Default.DirectionsCar, contentDescription = null)
                            Spacer(modifier = Modifier.width(8.dp))
                            Text("대여하기")
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun SpecRow(
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    label: String,
    value: String
) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.spacedBy(12.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Icon(
            icon,
            contentDescription = null,
            modifier = Modifier.size(24.dp),
            tint = MaterialTheme.colorScheme.onSurfaceVariant
        )
        Text(
            text = label,
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onSurfaceVariant,
            modifier = Modifier.weight(1f)
        )
        Text(
            text = value,
            style = MaterialTheme.typography.bodyMedium,
            fontWeight = FontWeight.SemiBold
        )
    }
}