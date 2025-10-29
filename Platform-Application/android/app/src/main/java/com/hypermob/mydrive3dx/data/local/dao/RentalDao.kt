package com.hypermob.mydrive3dx.data.local.dao

import androidx.room.*
import com.hypermob.mydrive3dx.data.local.entity.RentalEntity
import kotlinx.coroutines.flow.Flow

/**
 * Rental DAO (Room Database)
 *
 * 대여 정보 데이터 접근 객체
 */
@Dao
interface RentalDao {

    @Query("SELECT * FROM rentals WHERE userId = :userId ORDER BY createdAt DESC")
    fun getRentalsByUserId(userId: String): Flow<List<RentalEntity>>

    @Query("SELECT * FROM rentals WHERE rentalId = :rentalId")
    suspend fun getRentalById(rentalId: String): RentalEntity?

    @Query("SELECT * FROM rentals WHERE rentalId = :rentalId")
    fun getRentalByIdFlow(rentalId: String): Flow<RentalEntity?>

    @Query("SELECT * FROM rentals WHERE status IN (:statuses) ORDER BY startTime DESC")
    fun getRentalsByStatus(statuses: List<String>): Flow<List<RentalEntity>>

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertRental(rental: RentalEntity)

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertRentals(rentals: List<RentalEntity>)

    @Update
    suspend fun updateRental(rental: RentalEntity)

    @Delete
    suspend fun deleteRental(rental: RentalEntity)

    @Query("DELETE FROM rentals WHERE rentalId = :rentalId")
    suspend fun deleteRentalById(rentalId: String)

    @Query("DELETE FROM rentals")
    suspend fun deleteAllRentals()

    @Query("DELETE FROM rentals WHERE cachedAt < :timestamp")
    suspend fun deleteOldRentals(timestamp: Long)
}
