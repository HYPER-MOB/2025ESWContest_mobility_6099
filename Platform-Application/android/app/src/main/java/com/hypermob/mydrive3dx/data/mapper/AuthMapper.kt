package com.hypermob.mydrive3dx.data.mapper

import com.hypermob.mydrive3dx.data.remote.dto.AuthTokenDto
import com.hypermob.mydrive3dx.data.remote.dto.LoginRequestDto
import com.hypermob.mydrive3dx.data.remote.dto.RegisterRequestDto
import com.hypermob.mydrive3dx.data.remote.dto.UserDto
import com.hypermob.mydrive3dx.domain.model.*

/**
 * Auth Mapper
 *
 * DTO ↔ Domain Model 변환
 */

// DTO -> Domain
fun AuthTokenDto.toDomain(): AuthToken {
    return AuthToken(
        accessToken = accessToken,
        refreshToken = refreshToken,
        expiresIn = expiresIn
    )
}

fun UserDto.toDomain(): User {
    return User(
        userId = userId,
        email = email,
        name = name,
        phone = phone,
        drivingLicense = drivingLicense,
        profileImage = profileImage,
        createdAt = createdAt
    )
}

// Domain -> DTO
fun LoginRequest.toDto(): LoginRequestDto {
    return LoginRequestDto(
        email = email,
        password = password
    )
}

fun RegisterRequest.toDto(): RegisterRequestDto {
    return RegisterRequestDto(
        email = email,
        password = password,
        name = name,
        phone = phone,
        drivingLicense = drivingLicense
    )
}
