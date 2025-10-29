package com.hypermob.mydrive3dx.data.local.dao

import androidx.room.*
import com.hypermob.mydrive3dx.data.local.entity.VehicleEntity
import kotlinx.coroutines.flow.Flow

/**
 * Vehicle DAO (Room Database)
 *
 * 차량 정보 데이터 접근 객체
 */
@Dao
interface VehicleDao {

    @Query("SELECT * FROM vehicles ORDER BY model ASC")
    fun getAllVehicles(): Flow<List<VehicleEntity>>

    @Query("SELECT * FROM vehicles WHERE vehicleId = :vehicleId")
    suspend fun getVehicleById(vehicleId: String): VehicleEntity?

    @Query("SELECT * FROM vehicles WHERE vehicleId = :vehicleId")
    fun getVehicleByIdFlow(vehicleId: String): Flow<VehicleEntity?>

    @Query("SELECT * FROM vehicles WHERE status = :status ORDER BY model ASC")
    fun getVehiclesByStatus(status: String): Flow<List<VehicleEntity>>

    @Query("SELECT * FROM vehicles WHERE vehicleType = :type ORDER BY model ASC")
    fun getVehiclesByType(type: String): Flow<List<VehicleEntity>>

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertVehicle(vehicle: VehicleEntity)

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertVehicles(vehicles: List<VehicleEntity>)

    @Update
    suspend fun updateVehicle(vehicle: VehicleEntity)

    @Delete
    suspend fun deleteVehicle(vehicle: VehicleEntity)

    @Query("DELETE FROM vehicles WHERE vehicleId = :vehicleId")
    suspend fun deleteVehicleById(vehicleId: String)

    @Query("DELETE FROM vehicles")
    suspend fun deleteAllVehicles()

    @Query("DELETE FROM vehicles WHERE cachedAt < :timestamp")
    suspend fun deleteOldVehicles(timestamp: Long)
}
