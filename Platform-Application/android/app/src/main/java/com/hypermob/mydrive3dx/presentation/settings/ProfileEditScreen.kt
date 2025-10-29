package com.hypermob.mydrive3dx.presentation.settings

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel

/**
 * Profile Edit Screen
 *
 * 프로필 편집 화면 (간단 버전)
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ProfileEditScreen(
    viewModel: SettingsViewModel = hiltViewModel(),
    onNavigateBack: () -> Unit
) {
    val uiState by viewModel.uiState.collectAsState()
    val scrollState = rememberScrollState()

    var name by remember { mutableStateOf(uiState.user?.name ?: "") }
    var phone by remember { mutableStateOf(uiState.user?.phone ?: "") }
    var drivingLicense by remember { mutableStateOf(uiState.user?.drivingLicense ?: "") }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("프로필 편집") },
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
                .padding(16.dp)
                .verticalScroll(scrollState),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            // 프로필 이미지 (플레이스홀더)
            Card(modifier = Modifier.fillMaxWidth()) {
                Column(
                    modifier = Modifier.padding(16.dp),
                    verticalArrangement = Arrangement.spacedBy(8.dp),
                    horizontalAlignment = androidx.compose.ui.Alignment.CenterHorizontally
                ) {
                    Icon(
                        Icons.Default.AccountCircle,
                        contentDescription = null,
                        modifier = Modifier.size(100.dp),
                        tint = MaterialTheme.colorScheme.primary
                    )
                    TextButton(onClick = { /* TODO: 이미지 변경 */ }) {
                        Text("프로필 사진 변경")
                    }
                }
            }

            // 이메일 (수정 불가)
            OutlinedTextField(
                value = uiState.user?.email ?: "",
                onValueChange = {},
                label = { Text("이메일") },
                enabled = false,
                modifier = Modifier.fillMaxWidth()
            )

            // 이름
            OutlinedTextField(
                value = name,
                onValueChange = { name = it },
                label = { Text("이름") },
                modifier = Modifier.fillMaxWidth()
            )

            // 전화번호
            OutlinedTextField(
                value = phone,
                onValueChange = { phone = it },
                label = { Text("전화번호") },
                keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Phone),
                modifier = Modifier.fillMaxWidth()
            )

            // 운전면허번호
            OutlinedTextField(
                value = drivingLicense,
                onValueChange = { drivingLicense = it },
                label = { Text("운전면허번호 (선택)") },
                modifier = Modifier.fillMaxWidth()
            )

            Spacer(modifier = Modifier.height(8.dp))

            // 저장 버튼
            Button(
                onClick = {
                    // TODO: 프로필 업데이트 API 호출
                    onNavigateBack()
                },
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("저장")
            }
        }
    }
}
