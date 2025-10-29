package com.hypermob.mydrive3dx.presentation.rental

import androidx.compose.animation.core.*
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import com.hypermob.mydrive3dx.domain.model.Rental
import kotlinx.coroutines.delay

/**
 * 대여 승인 화면 (시나리오2)
 *
 * 대여 요청이 승인되면 표시되는 화면
 * 3초 후 자동으로 메인 화면으로 전환
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun RentalApprovalScreen(
    rental: Rental,
    onNavigateToHome: () -> Unit
) {
    // 3초 후 자동으로 홈으로 이동
    LaunchedEffect(Unit) {
        delay(3000)
        onNavigateToHome()
    }

    // 애니메이션 효과
    val infiniteTransition = rememberInfiniteTransition(label = "approval")
    val scale = infiniteTransition.animateFloat(
        initialValue = 0.8f,
        targetValue = 1.2f,
        animationSpec = infiniteRepeatable(
            animation = tween(1000, easing = FastOutSlowInEasing),
            repeatMode = RepeatMode.Reverse
        ),
        label = "scale"
    )

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("대여 승인") },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.primary,
                    titleContentColor = MaterialTheme.colorScheme.onPrimary
                )
            )
        }
    ) { paddingValues ->
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues),
            contentAlignment = Alignment.Center
        ) {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(24.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.spacedBy(24.dp)
            ) {
                // 성공 아이콘 (애니메이션)
                Icon(
                    Icons.Default.CheckCircle,
                    contentDescription = null,
                    modifier = Modifier.size(120.dp * scale.value),
                    tint = Color(0xFF4CAF50)
                )

                Text(
                    text = "대여가 승인되었습니다!",
                    style = MaterialTheme.typography.headlineMedium.copy(
                        fontWeight = FontWeight.Bold
                    ),
                    color = MaterialTheme.colorScheme.primary
                )

                // 대여 정보 카드
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    colors = CardDefaults.cardColors(
                        containerColor = MaterialTheme.colorScheme.primaryContainer
                    )
                ) {
                    Column(
                        modifier = Modifier.padding(20.dp),
                        verticalArrangement = Arrangement.spacedBy(12.dp)
                    ) {
                        RentalInfoRow(
                            label = "차량",
                            value = rental.vehicle?.let { "${it.manufacturer} ${it.model}" } ?: "정보 없음"
                        )
                        RentalInfoRow(
                            label = "대여 기간",
                            value = "${rental.startTime} ~ ${rental.endTime}"
                        )
                        RentalInfoRow(
                            label = "픽업 장소",
                            value = rental.pickupLocation
                        )
                        RentalInfoRow(
                            label = "총 비용",
                            value = "₩${String.format("%,.0f", rental.totalPrice)}"
                        )
                    }
                }

                // 다음 단계 안내
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    colors = CardDefaults.cardColors(
                        containerColor = MaterialTheme.colorScheme.secondaryContainer
                    )
                ) {
                    Row(
                        modifier = Modifier.padding(16.dp),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(12.dp)
                    ) {
                        Icon(
                            Icons.Default.Info,
                            contentDescription = null,
                            tint = MaterialTheme.colorScheme.onSecondaryContainer
                        )
                        Column {
                            Text(
                                text = "다음 단계",
                                style = MaterialTheme.typography.titleSmall,
                                fontWeight = FontWeight.Bold
                            )
                            Text(
                                text = "차량 인수 시 MFA 인증이 필요합니다",
                                style = MaterialTheme.typography.bodySmall
                            )
                        }
                    }
                }

                // 자동 전환 안내
                LinearProgressIndicator(
                    modifier = Modifier.fillMaxWidth()
                )
                Text(
                    text = "3초 후 메인 화면으로 이동합니다...",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }
    }
}

@Composable
private fun RentalInfoRow(
    label: String,
    value: String
) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(
            text = label,
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onPrimaryContainer.copy(alpha = 0.7f)
        )
        Text(
            text = value,
            style = MaterialTheme.typography.bodyMedium,
            fontWeight = FontWeight.SemiBold,
            color = MaterialTheme.colorScheme.onPrimaryContainer
        )
    }
}