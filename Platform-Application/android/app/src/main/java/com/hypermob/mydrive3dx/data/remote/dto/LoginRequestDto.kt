package com.hypermob.mydrive3dx.data.remote.dto

import kotlinx.serialization.Serializable

/**
 * Login Request DTO
 */
@Serializable
data class LoginRequestDto(
    val email: String,
    val password: String
)
