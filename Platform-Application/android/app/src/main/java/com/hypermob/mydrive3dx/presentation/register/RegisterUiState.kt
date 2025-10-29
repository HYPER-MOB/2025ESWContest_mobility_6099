package com.hypermob.mydrive3dx.presentation.register

/**
 * Register UI State
 *
 * 회원가입 화면의 상태
 */
data class RegisterUiState(
    val email: String = "",
    val password: String = "",
    val confirmPassword: String = "",
    val name: String = "",
    val phone: String = "",
    val drivingLicense: String = "",
    val isLoading: Boolean = false,
    val isSuccess: Boolean = false,
    val errorMessage: String? = null
)
