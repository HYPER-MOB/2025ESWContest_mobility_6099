package com.hypermob.mydrive3dx.presentation.rental

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.FilterList
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
 * Rental List Screen
 *
 * 대여 목록 화면
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun RentalListScreen(
    viewModel: RentalListViewModel = hiltViewModel(),
    onNavigateToDetail: (String) -> Unit,
    onNavigateToApproval: ((String) -> Unit)? = null
) {
    val uiState by viewModel.uiState.collectAsState()
    var showFilterDialog by remember { mutableStateOf(false) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("대여 목록") },
                actions = {
                    IconButton(onClick = { showFilterDialog = true }) {
                        Icon(Icons.Default.FilterList, contentDescription = "필터")
                    }
                }
            )
        }
    ) { paddingValues ->
        LazyColumn(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
                .padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
                // 필터 칩
                item {
                    FilterChips(
                        selectedFilter = uiState.selectedFilter,
                        onFilterSelected = { viewModel.setFilter(it) }
                    )
                }

                if (uiState.isLoading) {
                    item {
                        Box(
                            modifier = Modifier.fillMaxWidth(),
                            contentAlignment = Alignment.Center
                        ) {
                            CircularProgressIndicator()
                        }
                    }
                } else if (uiState.rentals.isEmpty()) {
                    item {
                        EmptyRentalList()
                    }
                } else {
                    items(uiState.rentals) { rental ->
                        RentalListItem(
                            rental = rental,
                            onClick = { onNavigateToDetail(rental.rentalId) },
                            onRequestRental = if (rental.status == RentalStatus.REQUESTED) {
                                { onNavigateToApproval?.invoke(rental.rentalId) }
                            } else null
                        )
                    }
                }
            }
        }

        // 필터 다이얼로그
        if (showFilterDialog) {
        FilterDialog(
            selectedFilter = uiState.selectedFilter,
            onFilterSelected = {
                viewModel.setFilter(it)
                showFilterDialog = false
            },
            onDismiss = { showFilterDialog = false }
        )
    }
}

@Composable
private fun FilterChips(
    selectedFilter: RentalStatus?,
    onFilterSelected: (RentalStatus?) -> Unit
) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        FilterChip(
            selected = selectedFilter == null,
            onClick = { onFilterSelected(null) },
            label = { Text("전체") }
        )
        FilterChip(
            selected = selectedFilter == RentalStatus.APPROVED,
            onClick = { onFilterSelected(RentalStatus.APPROVED) },
            label = { Text("승인됨") }
        )
        FilterChip(
            selected = selectedFilter == RentalStatus.PICKED,
            onClick = { onFilterSelected(RentalStatus.PICKED) },
            label = { Text("진행 중") }
        )
    }
}

@Composable
private fun RentalListItem(
    rental: Rental,
    onClick: () -> Unit,
    onRequestRental: (() -> Unit)? = null
) {
    Card(
        onClick = onClick,
        modifier = Modifier.fillMaxWidth()
    ) {
        Column(
            modifier = Modifier.padding(16.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column(modifier = Modifier.weight(1f)) {
                    Text(
                        text = rental.vehicle?.model ?: "차량 정보 없음",
                        style = MaterialTheme.typography.titleMedium
                    )
                    Text(
                        text = rental.vehicle?.licensePlate ?: "",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }

                StatusBadge(status = rental.status)
            }

            Spacer(modifier = Modifier.height(8.dp))

            Text(
                text = "${rental.startTime} ~ ${rental.endTime}",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )

            Text(
                text = "₩${String.format("%,.0f", rental.totalPrice)}",
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.primary
            )

            // 시나리오2: 대여 요청 버튼
            if (onRequestRental != null) {
                Spacer(modifier = Modifier.height(12.dp))
                Button(
                    onClick = onRequestRental,
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("대여 요청")
                }
            }
        }
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
            style = MaterialTheme.typography.labelSmall,
            color = color,
            modifier = Modifier.padding(horizontal = 8.dp, vertical = 4.dp)
        )
    }
}

@Composable
private fun EmptyRentalList() {
    Box(
        modifier = Modifier
            .fillMaxWidth()
            .padding(32.dp),
        contentAlignment = Alignment.Center
    ) {
        Text(
            text = "대여 내역이 없습니다",
            style = MaterialTheme.typography.bodyLarge,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )
    }
}

@Composable
private fun FilterDialog(
    selectedFilter: RentalStatus?,
    onFilterSelected: (RentalStatus?) -> Unit,
    onDismiss: () -> Unit
) {
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text("필터 선택") },
        text = {
            Column {
                listOf(
                    null to "전체",
                    RentalStatus.REQUESTED to "요청됨",
                    RentalStatus.APPROVED to "승인됨",
                    RentalStatus.PICKED to "진행 중",
                    RentalStatus.RETURNED to "반납 완료",
                    RentalStatus.CANCELED to "취소됨"
                ).forEach { (status, label) ->
                    TextButton(
                        onClick = { onFilterSelected(status) },
                        modifier = Modifier.fillMaxWidth()
                    ) {
                        Text(label)
                    }
                }
            }
        },
        confirmButton = {
            TextButton(onClick = onDismiss) {
                Text("닫기")
            }
        }
    )
}
