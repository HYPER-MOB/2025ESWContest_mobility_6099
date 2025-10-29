package com.hypermob.mydrive3dx.domain.repository

import com.hypermob.mydrive3dx.domain.model.*
import kotlinx.coroutines.flow.Flow

/**
 * Vehicle Control Repository Interface
 *
 * 차량 제어 관련 Repository 인터페이스 (Domain Layer)
 */
interface VehicleControlRepository {

    /**
     * 차량 제어 상태 조회
     */
    suspend fun getVehicleControl(rentalId: String): Flow<Result<VehicleControl>>

    /**
     * 문 잠금
     */
    suspend fun lockDoors(rentalId: String): Flow<Result<ControlCommandResponse>>

    /**
     * 문 잠금 해제
     */
    suspend fun unlockDoors(rentalId: String): Flow<Result<ControlCommandResponse>>

    /**
     * 시동 켜기
     */
    suspend fun startEngine(rentalId: String): Flow<Result<ControlCommandResponse>>

    /**
     * 시동 끄기
     */
    suspend fun stopEngine(rentalId: String): Flow<Result<ControlCommandResponse>>

    /**
     * 에어컨 켜기
     */
    suspend fun turnOnClimate(rentalId: String, temperature: Double): Flow<Result<ControlCommandResponse>>

    /**
     * 에어컨 끄기
     */
    suspend fun turnOffClimate(rentalId: String): Flow<Result<ControlCommandResponse>>

    /**
     * 경적 울리기
     */
    suspend fun honkHorn(rentalId: String): Flow<Result<ControlCommandResponse>>

    /**
     * 라이트 깜빡이기
     */
    suspend fun flashLights(rentalId: String): Flow<Result<ControlCommandResponse>>

    /**
     * 일반 제어 명령 실행
     */
    suspend fun executeCommand(request: ControlCommandRequest): Flow<Result<ControlCommandResponse>>
}
