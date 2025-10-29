package com.hypermob.mydrive3dx.data.repository

import com.hypermob.mydrive3dx.data.remote.api.VehicleApi
import com.hypermob.mydrive3dx.domain.model.Vehicle
import com.hypermob.mydrive3dx.domain.repository.VehicleRepository
import javax.inject.Inject

/**
 * Vehicle Repository Implementation
 * 차량 관련 데이터 저장소 구현
 */
class VehicleRepositoryImpl @Inject constructor(
    private val api: VehicleApi
) : VehicleRepository {

    override suspend fun getVehicles(status: String?, location: String?): List<Vehicle> {
        return api.getVehicles(status, location)
    }

    override suspend fun getVehicle(carId: String): Vehicle {
        return api.getVehicle(carId)
    }
}
