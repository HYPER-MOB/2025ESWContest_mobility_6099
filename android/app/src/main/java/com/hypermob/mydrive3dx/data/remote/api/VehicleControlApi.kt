package com.hypermob.mydrive3dx.data.remote.api

import com.hypermob.mydrive3dx.data.remote.dto.ApiResponse
import com.hypermob.mydrive3dx.data.remote.dto.ControlCommandRequestDto
import com.hypermob.mydrive3dx.data.remote.dto.ControlCommandResponseDto
import com.hypermob.mydrive3dx.data.remote.dto.VehicleControlDto
import com.hypermob.mydrive3dx.domain.model.*
import retrofit2.http.*

/**
 * Vehicle Control API Interface
 *
 * 차량 제어 관련 API 엔드포인트
 */
interface VehicleControlApi {

    /**
     * 차량 제어 상태 조회
     * GET /api/rentals/{rentalId}/control
     */
    @GET("rentals/{rentalId}/control")
    suspend fun getVehicleControl(
        @Path("rentalId") rentalId: String
    ): ApiResponse<VehicleControlDto>

    /**
     * 제어 명령 실행
     * POST /api/rentals/{rentalId}/control/command
     */
    @POST("rentals/{rentalId}/control/command")
    suspend fun executeCommand(
        @Path("rentalId") rentalId: String,
        @Body request: ControlCommandRequestDto
    ): ApiResponse<ControlCommandResponseDto>

    // === 새로 추가된 엔드포인트 ===

    /**
     * POST /vehicles/{car_id}/settings/apply
     * 차량 설정 자동 적용
     */
    @POST("vehicles/{car_id}/settings/apply")
    suspend fun applySettings(
        @Path("car_id") carId: String,
        @Body request: ApplySettingsRequest
    ): ApplySettingsResponse

    /**
     * GET /vehicles/{car_id}/settings/current
     * 현재 설정 조회
     */
    @GET("vehicles/{car_id}/settings/current")
    suspend fun getCurrentSettings(
        @Path("car_id") carId: String
    ): UserVehicleSettings

    /**
     * POST /vehicles/{car_id}/settings/manual
     * 수동 조작 기록
     */
    @POST("vehicles/{car_id}/settings/manual")
    suspend fun recordManualAdjustment(
        @Path("car_id") carId: String,
        @Body settings: UserVehicleSettings
    )

    /**
     * GET /vehicles/{car_id}/settings/history
     * 설정 히스토리 조회
     */
    @GET("vehicles/{car_id}/settings/history")
    suspend fun getSettingsHistory(
        @Path("car_id") carId: String,
        @Query("user_id") userId: String? = null
    ): List<SettingsHistoryItem>
}
