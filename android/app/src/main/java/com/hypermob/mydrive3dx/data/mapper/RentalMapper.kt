package com.hypermob.mydrive3dx.data.mapper

import com.hypermob.mydrive3dx.data.remote.dto.MfaStatusDto
import com.hypermob.mydrive3dx.data.remote.dto.RentalDto
import com.hypermob.mydrive3dx.data.remote.dto.RentalRequestDto
import com.hypermob.mydrive3dx.data.remote.dto.VehicleDto
import com.hypermob.mydrive3dx.domain.model.*

/**
 * Rental Mapper
 *
 * DTO ↔ Domain Model 변환
 */

// DTO -> Domain
fun VehicleDto.toDomain(): Vehicle {
    return Vehicle(
        vehicleId = vehicleId,
        model = model,
        manufacturer = manufacturer ?: "Unknown",
        year = year,
        color = color,
        licensePlate = licensePlate,
        vehicleType = parseVehicleType(model), // Parse from model or default
        fuelType = parseFuelType(fuelType),
        transmission = parseTransmission(transmission),
        seatingCapacity = seatingCapacity ?: 5,
        pricePerHour = pricePerHour,
        pricePerDay = pricePerDay ?: (pricePerHour * 20.0), // Use DTO value or calculate
        imageUrl = imageUrl,
        location = location,
        latitude = latitude,
        longitude = longitude,
        status = parseVehicleStatus(status),
        features = features ?: emptyList()
    )
}

// Helper functions to parse enum values safely
private fun parseVehicleStatus(status: String): VehicleStatus {
    return try {
        VehicleStatus.valueOf(status.uppercase())
    } catch (e: Exception) {
        VehicleStatus.AVAILABLE // Default
    }
}

private fun parseVehicleType(model: String): VehicleType {
    return when {
        model.contains("SUV", ignoreCase = true) -> VehicleType.SUV
        model.contains("Truck", ignoreCase = true) -> VehicleType.TRUCK
        else -> VehicleType.SEDAN
    }
}

private fun parseFuelType(fuelType: String?): FuelType {
    return when (fuelType?.uppercase()) {
        "ELECTRIC" -> FuelType.ELECTRIC
        "DIESEL" -> FuelType.DIESEL
        "HYBRID" -> FuelType.HYBRID
        else -> FuelType.GASOLINE
    }
}

private fun parseTransmission(transmission: String?): TransmissionType {
    return when (transmission?.uppercase()) {
        "MANUAL" -> TransmissionType.MANUAL
        else -> TransmissionType.AUTOMATIC
    }
}

fun MfaStatusDto.toDomain(): MfaStatus {
    return MfaStatus(
        faceVerified = faceVerified,
        nfcVerified = nfcVerified,
        bleVerified = bleVerified,
        allPassed = allPassed
    )
}

fun RentalDto.toDomain(): Rental {
    return Rental(
        rentalId = rentalId,
        vehicleId = vehicleId,
        userId = userId,
        vehicle = vehicle?.toDomain(),
        startTime = startTime,
        endTime = endTime,
        status = RentalStatus.fromString(status),
        totalPrice = totalPrice ?: 0.0,
        pickupLocation = pickupLocation ?: "",
        returnLocation = returnLocation ?: "",
        createdAt = createdAt ?: "",
        updatedAt = updatedAt ?: "",
        mfaStatus = mfaStatus?.toDomain()
    )
}

// Domain -> DTO
fun RentalRequest.toDto(userId: String): RentalRequestDto {
    return RentalRequestDto(
        userId = userId,
        vehicleId = vehicleId,
        startTime = startTime,
        endTime = endTime
    )
}
