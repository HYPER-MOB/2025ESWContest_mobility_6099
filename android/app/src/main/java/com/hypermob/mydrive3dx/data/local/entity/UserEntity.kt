package com.hypermob.mydrive3dx.data.local.entity

import androidx.room.Entity
import androidx.room.PrimaryKey

/**
 * User Entity (Room Database)
 *
 * 오프라인 캐싱을 위한 사용자 정보 엔티티
 */
@Entity(tableName = "users")
data class UserEntity(
    @PrimaryKey
    val userId: String,
    val email: String,
    val name: String,
    val phone: String,
    val drivingLicense: String?,
    val profileImageUrl: String?,
    val createdAt: String,

    // Timestamp for cache management
    val cachedAt: Long = System.currentTimeMillis()
)
