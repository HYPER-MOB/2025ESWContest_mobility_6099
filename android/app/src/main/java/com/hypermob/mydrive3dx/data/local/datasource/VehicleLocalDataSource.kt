package com.hypermob.mydrive3dx.data.local.datasource

import com.hypermob.mydrive3dx.data.local.dao.VehicleDao
import com.hypermob.mydrive3dx.data.mapper.toDomain
import com.hypermob.mydrive3dx.data.mapper.toEntity
import com.hypermob.mydrive3dx.domain.model.Vehicle
import com.hypermob.mydrive3dx.domain.model.VehicleStatus
import com.hypermob.mydrive3dx.domain.model.VehicleType
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map
import javax.inject.Inject
import javax.inject.Singleton

/**
 * Vehicle Local Data Source
 *
 * 차량 정보 로컬 데이터 소스 (Room)
 */
interface VehicleLocalDataSource {
    fun getAllVehicles(): Flow<List<Vehicle>>
    fun getVehicleById(vehicleId: String): Flow<Vehicle?>
    suspend fun getVehicleByIdOnce(vehicleId: String): Vehicle?
    fun getVehiclesByStatus(status: VehicleStatus): Flow<List<Vehicle>>
    fun getVehiclesByType(type: VehicleType): Flow<List<Vehicle>>
    suspend fun saveVehicle(vehicle: Vehicle)
    suspend fun saveVehicles(vehicles: List<Vehicle>)
    suspend fun updateVehicle(vehicle: Vehicle)
    suspend fun deleteVehicle(vehicleId: String)
    suspend fun clearAllVehicles()
    suspend fun clearOldVehicles(timestamp: Long)
}

@Singleton
class VehicleLocalDataSourceImpl @Inject constructor(
    private val vehicleDao: VehicleDao
) : VehicleLocalDataSource {

    override fun getAllVehicles(): Flow<List<Vehicle>> {
        return vehicleDao.getAllVehicles().map { entities ->
            entities.map { it.toDomain() }
        }
    }

    override fun getVehicleById(vehicleId: String): Flow<Vehicle?> {
        return vehicleDao.getVehicleByIdFlow(vehicleId).map { it?.toDomain() }
    }

    override suspend fun getVehicleByIdOnce(vehicleId: String): Vehicle? {
        return vehicleDao.getVehicleById(vehicleId)?.toDomain()
    }

    override fun getVehiclesByStatus(status: VehicleStatus): Flow<List<Vehicle>> {
        return vehicleDao.getVehiclesByStatus(status.name).map { entities ->
            entities.map { it.toDomain() }
        }
    }

    override fun getVehiclesByType(type: VehicleType): Flow<List<Vehicle>> {
        return vehicleDao.getVehiclesByType(type.name).map { entities ->
            entities.map { it.toDomain() }
        }
    }

    override suspend fun saveVehicle(vehicle: Vehicle) {
        vehicleDao.insertVehicle(vehicle.toEntity())
    }

    override suspend fun saveVehicles(vehicles: List<Vehicle>) {
        vehicleDao.insertVehicles(vehicles.map { it.toEntity() })
    }

    override suspend fun updateVehicle(vehicle: Vehicle) {
        vehicleDao.updateVehicle(vehicle.toEntity())
    }

    override suspend fun deleteVehicle(vehicleId: String) {
        vehicleDao.deleteVehicleById(vehicleId)
    }

    override suspend fun clearAllVehicles() {
        vehicleDao.deleteAllVehicles()
    }

    override suspend fun clearOldVehicles(timestamp: Long) {
        vehicleDao.deleteOldVehicles(timestamp)
    }
}
