package com.hypermob.mydrive3dx.data.mapper

import com.hypermob.mydrive3dx.data.remote.dto.*
import com.hypermob.mydrive3dx.domain.model.*

/**
 * Body Scan Mapper
 *
 * DTO ↔ Domain 변환
 */

// BodyScan
fun BodyScanDto.toDomain(): BodyScan {
    return BodyScan(
        scanId = scanId ?: "",
        userId = userId ?: "",
        timestamp = timestamp ?: "",
        sittingHeight = sittingHeight ?: 0.0,
        shoulderWidth = shoulderWidth ?: 0.0,
        armLength = armLength ?: 0.0,
        headHeight = headHeight ?: 0.0,
        eyeHeight = eyeHeight ?: 0.0,
        legLength = legLength ?: 0.0,
        torsoLength = torsoLength ?: 0.0,
        weight = weight,
        height = height
    )
}

fun BodyScan.toDto(): BodyScanDto {
    return BodyScanDto(
        scanId = scanId,
        userId = userId,
        timestamp = timestamp,
        sittingHeight = sittingHeight,
        shoulderWidth = shoulderWidth,
        armLength = armLength,
        headHeight = headHeight,
        eyeHeight = eyeHeight,
        legLength = legLength,
        torsoLength = torsoLength,
        weight = weight,
        height = height
    )
}

// VehicleSettings
fun VehicleSettingsDto.toDomain(): VehicleSettings {
    return VehicleSettings(
        seatFrontBack = seatFrontBack ?: 0.0,
        seatUpDown = seatUpDown ?: 0.0,
        seatBackAngle = seatBackAngle ?: 0.0,
        seatHeadrestHeight = seatHeadrestHeight ?: 0.0,
        leftMirrorAngle = leftMirrorAngle ?: 0.0,
        rightMirrorAngle = rightMirrorAngle ?: 0.0,
        steeringWheelHeight = steeringWheelHeight ?: 0.0,
        steeringWheelDepth = steeringWheelDepth ?: 0.0,
        vehicleModel = vehicleModel,
        calibrationVersion = calibrationVersion
    )
}

fun VehicleSettings.toDto(): VehicleSettingsDto {
    return VehicleSettingsDto(
        seatFrontBack = seatFrontBack,
        seatUpDown = seatUpDown,
        seatBackAngle = seatBackAngle,
        seatHeadrestHeight = seatHeadrestHeight,
        leftMirrorAngle = leftMirrorAngle,
        rightMirrorAngle = rightMirrorAngle,
        steeringWheelHeight = steeringWheelHeight,
        steeringWheelDepth = steeringWheelDepth,
        vehicleModel = vehicleModel,
        calibrationVersion = calibrationVersion
    )
}

// BodyProfile
fun BodyProfileDto.toDomain(): BodyProfile {
    return BodyProfile(
        profileId = profileId ?: "",
        userId = userId ?: "",
        userName = userName ?: "",
        bodyScan = bodyScan?.toDomain() ?: BodyScan("", userId ?: "", "", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, null, null),
        vehicleSettings = vehicleSettings?.toDomain() ?: VehicleSettings(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, null, "1.0"),
        createdAt = createdAt ?: "",
        updatedAt = updatedAt ?: "",
        isActive = isActive
    )
}

fun BodyProfile.toDto(): BodyProfileDto {
    return BodyProfileDto(
        profileId = profileId,
        userId = userId,
        userName = userName,
        bodyScan = bodyScan.toDto(),
        vehicleSettings = vehicleSettings.toDto(),
        createdAt = createdAt,
        updatedAt = updatedAt,
        isActive = isActive
    )
}

// BodyScanRequest
fun BodyScanRequest.toDto(): BodyScanRequestDto {
    return BodyScanRequestDto(
        userId = userId,
        vehicleId = vehicleId
    )
}

// VehicleSettingResult
fun VehicleSettingResultDto.toDomain(): VehicleSettingResult {
    return VehicleSettingResult(
        success = success ?: false,
        profileId = profileId ?: "",
        appliedSettings = appliedSettings?.toDomain() ?: VehicleSettings(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, null, "1.0"),
        estimatedTime = estimatedTime ?: 0,
        message = message
    )
}
