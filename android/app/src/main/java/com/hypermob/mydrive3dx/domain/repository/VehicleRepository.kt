package com.hypermob.mydrive3dx.domain.repository

import com.hypermob.mydrive3dx.domain.model.Vehicle

/**
 * Vehicle Repository Interface
 * 차량 관련 데이터 저장소 인터페이스
 */
interface VehicleRepository {
    suspend fun getVehicles(status: String? = null, location: String? = null): List<Vehicle>
    suspend fun getVehicle(carId: String): Vehicle
}
