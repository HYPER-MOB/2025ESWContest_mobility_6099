package com.hypermob.mydrive3dx.data.remote.dto

import kotlinx.serialization.Serializable

/**
 * Register Request DTO
 */
@Serializable
data class RegisterRequestDto(
    val email: String,
    val password: String,
    val name: String,
    val phone: String,
    val drivingLicense: String? = null
)
