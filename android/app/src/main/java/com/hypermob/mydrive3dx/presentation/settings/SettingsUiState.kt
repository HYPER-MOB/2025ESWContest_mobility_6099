package com.hypermob.mydrive3dx.presentation.settings

import com.hypermob.mydrive3dx.domain.model.User

/**
 * Settings UI State
 *
 * 설정 화면의 상태
 */
data class SettingsUiState(
    val user: User? = null,
    val isLoading: Boolean = false,
    val errorMessage: String? = null
)
