package com.hypermob.mydrive3dx.data.remote.dto

import kotlinx.serialization.Serializable

/**
 * User Response DTO
 */
@Serializable
data class UserDto(
    val userId: String,
    val email: String,
    val name: String,
    val phone: String,
    val drivingLicense: String? = null,
    val profileImage: String? = null,
    val createdAt: String,
    val updatedAt: String
)
