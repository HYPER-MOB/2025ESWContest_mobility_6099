package com.hypermob.mydrive3dx.data.remote.api

import com.hypermob.mydrive3dx.domain.model.Vehicle
import retrofit2.http.GET
import retrofit2.http.Path
import retrofit2.http.Query

/**
 * Vehicle API Interface
 * 차량 관련 API
 */
interface VehicleApi {

    /**
     * GET /vehicles
     * 차량 목록 조회
     */
    @GET("vehicles")
    suspend fun getVehicles(
        @Query("status") status: String? = null,
        @Query("location") location: String? = null
    ): List<Vehicle>

    /**
     * GET /vehicles/{car_id}
     * 특정 차량 조회
     */
    @GET("vehicles/{car_id}")
    suspend fun getVehicle(
        @Path("car_id") carId: String
    ): Vehicle
}
