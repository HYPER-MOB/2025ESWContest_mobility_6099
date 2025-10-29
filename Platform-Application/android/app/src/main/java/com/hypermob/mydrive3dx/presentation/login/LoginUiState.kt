package com.hypermob.mydrive3dx.presentation.login

/**
 * Login UI State
 *
 * 로그인 화면의 상태
 */
data class LoginUiState(
    val email: String = "",
    val password: String = "",
    val isLoading: Boolean = false,
    val isSuccess: Boolean = false,
    val errorMessage: String? = null
)
