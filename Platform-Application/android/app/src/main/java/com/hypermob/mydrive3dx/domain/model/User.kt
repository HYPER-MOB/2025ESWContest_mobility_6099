package com.hypermob.mydrive3dx.domain.model

/**
 * User Domain Model
 *
 * 사용자 도메인 엔티티
 */
data class User(
    val userId: String,
    val email: String,
    val name: String,
    val phone: String,
    val drivingLicense: String?,
    val profileImage: String?,
    val createdAt: String
)
