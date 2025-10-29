package com.hypermob.mydrive3dx.presentation.rental

import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import com.hypermob.mydrive3dx.domain.model.Rental
import com.hypermob.mydrive3dx.domain.model.RentalStatus
import com.hypermob.mydrive3dx.presentation.theme.StatusColors

/**
 * Rental Detail Screen
 *
 * 대여 상세 화면
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun RentalDetailScreen(
    viewModel: RentalDetailViewModel = hiltViewModel(),
    onNavigateBack: () -> Unit,
    onNavigateToMfa: (String) -> Unit
) {
    val uiState by viewModel.uiState.collectAsState()

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("대여 상세") },
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
            when {
                uiState.isLoading -> {
                    CircularProgressIndicator(
                        modifier = Modifier.align(Alignment.Center)
                    )
                }
                uiState.rental != null -> {
                    RentalDetailContent(
                        rental = uiState.rental!!,
                        onNavigateToMfa = onNavigateToMfa
                    )
                }
                uiState.errorMessage != null -> {
                    Text(
                        text = uiState.errorMessage!!,
                        color = MaterialTheme.colorScheme.error,
                        modifier = Modifier.align(Alignment.Center)
                    )
                }
            }
        }
    }
}

@Composable
private fun RentalDetailContent(
    rental: Rental,
    onNavigateToMfa: (String) -> Unit
) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        // 차량 정보 카드
        Card(modifier = Modifier.fillMaxWidth()) {
            Column(modifier = Modifier.padding(16.dp)) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Column {
                        Text(
                            text = rental.vehicle?.model ?: "차량 정보 없음",
                            style = MaterialTheme.typography.headlineSmall
                        )
                        Text(
                            text = rental.vehicle?.licensePlate ?: "",
                            style = MaterialTheme.typography.bodyMedium,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                    }

                    StatusBadge(status = rental.status)
                }

                Spacer(modifier = Modifier.height(8.dp))

                Divider()

                Spacer(modifier = Modifier.height(8.dp))

                InfoRow("모델", rental.vehicle?.model ?: "-")
                InfoRow("색상", rental.vehicle?.color ?: "-")
                InfoRow("연식", rental.vehicle?.year?.toString() ?: "-")
                InfoRow("위치", rental.vehicle?.location ?: "-")
            }
        }

        // 대여 정보 카드
        Card(modifier = Modifier.fillMaxWidth()) {
            Column(modifier = Modifier.padding(16.dp)) {
                Text(
                    text = "대여 정보",
                    style = MaterialTheme.typography.titleMedium
                )

                Spacer(modifier = Modifier.height(8.dp))

                InfoRow("대여 시작", rental.startTime)
                InfoRow("대여 종료", rental.endTime)
                InfoRow("인수 장소", rental.pickupLocation)
                InfoRow("반납 장소", rental.returnLocation)
                InfoRow("대여 금액", "₩${String.format("%,.0f", rental.totalPrice)}")
            }
        }

        // MFA 상태 카드 (APPROVED 상태일 때)
        if (rental.status == RentalStatus.APPROVED) {
            Card(
                modifier = Modifier.fillMaxWidth(),
                colors = CardDefaults.cardColors(
                    containerColor = MaterialTheme.colorScheme.primaryContainer
                )
            ) {
                Column(modifier = Modifier.padding(16.dp)) {
                    Text(
                        text = "차량 인수 준비",
                        style = MaterialTheme.typography.titleMedium,
                        color = MaterialTheme.colorScheme.onPrimaryContainer
                    )

                    Spacer(modifier = Modifier.height(8.dp))

                    Text(
                        text = "MFA 인증을 완료하고 차량을 인수하세요",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onPrimaryContainer
                    )

                    Spacer(modifier = Modifier.height(16.dp))

                    Button(
                        onClick = { onNavigateToMfa(rental.rentalId) },
                        modifier = Modifier.fillMaxWidth()
                    ) {
                        Text("MFA 인증 시작")
                    }
                }
            }
        }
    }
}

@Composable
private fun InfoRow(label: String, value: String) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(
            text = label,
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )
        Text(
            text = value,
            style = MaterialTheme.typography.bodyMedium
        )
    }
}

@Composable
private fun StatusBadge(status: RentalStatus) {
    val (text, color) = when (status) {
        RentalStatus.REQUESTED -> "요청됨" to StatusColors.Requested
        RentalStatus.APPROVED -> "승인됨" to StatusColors.Approved
        RentalStatus.PICKED -> "인수함" to StatusColors.Picked
        RentalStatus.RETURNED -> "반납함" to StatusColors.Returned
        RentalStatus.CANCELED -> "취소됨" to StatusColors.Canceled
    }

    Surface(
        color = color.copy(alpha = 0.2f),
        shape = MaterialTheme.shapes.small
    ) {
        Text(
            text = text,
            style = MaterialTheme.typography.labelMedium,
            color = color,
            modifier = Modifier.padding(horizontal = 12.dp, vertical = 6.dp)
        )
    }
}
