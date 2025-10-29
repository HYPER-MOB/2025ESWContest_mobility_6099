package com.hypermob.mydrive3dx.domain.model

/**
 * Register Request Domain Model
 */
data class RegisterRequest(
    val email: String,
    val password: String,
    val name: String,
    val phone: String,
    val drivingLicense: String?
)
