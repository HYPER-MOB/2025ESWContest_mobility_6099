package com.hypermob.mydrive3dx.domain.model

/**
 * Auth Token Domain Model
 *
 * JWT 인증 토큰
 */
data class AuthToken(
    val accessToken: String,
    val refreshToken: String,
    val expiresIn: Long // seconds
)
