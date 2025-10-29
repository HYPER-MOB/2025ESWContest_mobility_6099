package com.hypermob.mydrive3dx.data.mapper

import com.hypermob.mydrive3dx.data.local.entity.RentalEntity
import com.hypermob.mydrive3dx.data.local.entity.UserEntity
import com.hypermob.mydrive3dx.data.local.entity.VehicleEntity
import com.hypermob.mydrive3dx.domain.model.*
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.Json

/**
 * Entity Mapper
 *
 * Domain Model ↔ Room Entity 변환
 */

// User
fun User.toEntity(): UserEntity {
    return UserEntity(
        userId = userId,
        email = email,
        name = name,
        phone = phone,
        drivingLicense = drivingLicense,
        profileImageUrl = profileImage,
        createdAt = createdAt
    )
}

fun UserEntity.toDomain(): User {
    return User(
        userId = userId,
        email = email,
        name = name,
        phone = phone,
        drivingLicense = drivingLicense,
        profileImage = profileImageUrl,
        createdAt = createdAt
    )
}

// Vehicle
fun Vehicle.toEntity(): VehicleEntity {
    return VehicleEntity(
        vehicleId = vehicleId,
        model = model,
        manufacturer = manufacturer,
        year = year,
        color = color,
        licensePlate = licensePlate,
        vehicleType = vehicleType.name,
        fuelType = fuelType.name,
        transmission = transmission.name,
        seatingCapacity = seatingCapacity,
        pricePerHour = pricePerHour,
        pricePerDay = pricePerDay,
        imageUrl = imageUrl,
        location = location,
        latitude = latitude,
        longitude = longitude,
        status = status.name,
        features = Json.encodeToString(features)
    )
}

fun VehicleEntity.toDomain(): Vehicle {
    return Vehicle(
        vehicleId = vehicleId,
        model = model,
        manufacturer = manufacturer,
        year = year,
        color = color,
        licensePlate = licensePlate,
        vehicleType = VehicleType.valueOf(vehicleType),
        fuelType = FuelType.valueOf(fuelType),
        transmission = TransmissionType.valueOf(transmission),
        seatingCapacity = seatingCapacity,
        pricePerHour = pricePerHour,
        pricePerDay = pricePerDay,
        imageUrl = imageUrl,
        location = location,
        latitude = latitude,
        longitude = longitude,
        status = VehicleStatus.valueOf(status),
        features = try {
            Json.decodeFromString<List<String>>(features ?: "[]")
        } catch (e: Exception) {
            emptyList()
        }
    )
}

// Rental
fun Rental.toEntity(): RentalEntity {
    return RentalEntity(
        rentalId = rentalId,
        vehicleId = vehicleId,
        userId = userId,
        startTime = startTime,
        endTime = endTime,
        status = status.name,
        totalPrice = totalPrice,
        pickupLocation = pickupLocation,
        returnLocation = returnLocation,
        createdAt = createdAt,
        updatedAt = updatedAt,
        vehicleModel = vehicle?.model,
        vehicleLicensePlate = vehicle?.licensePlate,
        vehicleColor = vehicle?.color,
        vehicleYear = vehicle?.year,
        vehicleImageUrl = vehicle?.imageUrl
    )
}

fun RentalEntity.toDomain(): Rental {
    // Note: This creates a partial Vehicle object
    // For full vehicle details, need to fetch from VehicleDao
    return Rental(
        rentalId = rentalId,
        vehicleId = vehicleId,
        userId = userId,
        vehicle = Vehicle(
            vehicleId = vehicleId,
            model = vehicleModel ?: "Unknown",
            manufacturer = "Unknown",
            year = vehicleYear ?: 0,
            color = vehicleColor ?: "Unknown",
            licensePlate = vehicleLicensePlate ?: "Unknown",
            vehicleType = VehicleType.SEDAN,
            fuelType = FuelType.GASOLINE,
            transmission = TransmissionType.AUTOMATIC,
            seatingCapacity = 5,
            pricePerHour = 0.0,
            pricePerDay = 0.0,
            imageUrl = vehicleImageUrl,
            location = pickupLocation,
            latitude = null,
            longitude = null,
            status = VehicleStatus.AVAILABLE,
            features = emptyList()
        ),
        startTime = startTime,
        endTime = endTime,
        status = RentalStatus.valueOf(status),
        totalPrice = totalPrice,
        pickupLocation = pickupLocation,
        returnLocation = returnLocation,
        createdAt = createdAt,
        updatedAt = updatedAt
    )
}
