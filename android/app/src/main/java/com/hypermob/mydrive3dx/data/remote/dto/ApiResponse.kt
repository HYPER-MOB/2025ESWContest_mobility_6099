package com.hypermob.mydrive3dx.data.remote.dto

import kotlinx.serialization.Serializable

/**
 * Generic API Response Wrapper
 */
@Serializable
data class ApiResponse<T>(
    val success: Boolean? = null,
    val data: T? = null,
    val message: String? = null,
    val error: ErrorDto? = null
)

/**
 * Error DTO
 */
@Serializable
data class ErrorDto(
    val code: String,
    val message: String,
    val details: String? = null
)
