package com.hypermob.mydrive3dx.data.local.entity

import androidx.room.Entity
import androidx.room.PrimaryKey

/**
 * Vehicle Entity (Room Database)
 *
 * 오프라인 캐싱을 위한 차량 정보 엔티티
 */
@Entity(tableName = "vehicles")
data class VehicleEntity(
    @PrimaryKey
    val vehicleId: String,
    val model: String,
    val manufacturer: String,
    val year: Int,
    val color: String,
    val licensePlate: String,
    val vehicleType: String, // VehicleType enum name
    val fuelType: String, // FuelType enum name
    val transmission: String, // TransmissionType enum name
    val seatingCapacity: Int,
    val pricePerHour: Double,
    val pricePerDay: Double,
    val imageUrl: String?,
    val location: String,
    val latitude: Double?,
    val longitude: Double?,
    val status: String, // VehicleStatus enum name
    val features: String?, // JSON string of features list

    // Timestamp for cache management
    val cachedAt: Long = System.currentTimeMillis()
)
