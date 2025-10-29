package com.hypermob.mydrive3dx.data.remote.dto

import kotlinx.serialization.Serializable

/**
 * Auth Token Response DTO
 */
@Serializable
data class AuthTokenDto(
    val accessToken: String,
    val refreshToken: String,
    val expiresIn: Long
)
