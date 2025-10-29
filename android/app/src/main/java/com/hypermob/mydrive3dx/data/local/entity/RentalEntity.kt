package com.hypermob.mydrive3dx.data.local.entity

import androidx.room.Entity
import androidx.room.PrimaryKey

/**
 * Rental Entity (Room Database)
 *
 * 오프라인 캐싱을 위한 대여 정보 엔티티
 */
@Entity(tableName = "rentals")
data class RentalEntity(
    @PrimaryKey
    val rentalId: String,
    val vehicleId: String,
    val userId: String,
    val startTime: String,
    val endTime: String,
    val status: String, // RentalStatus enum name
    val totalPrice: Double,
    val pickupLocation: String,
    val returnLocation: String,
    val createdAt: String,
    val updatedAt: String,

    // Nested data (JSON string or separate fields)
    val vehicleModel: String?,
    val vehicleLicensePlate: String?,
    val vehicleColor: String?,
    val vehicleYear: Int?,
    val vehicleImageUrl: String?,

    // Timestamp for cache management
    val cachedAt: Long = System.currentTimeMillis()
)
