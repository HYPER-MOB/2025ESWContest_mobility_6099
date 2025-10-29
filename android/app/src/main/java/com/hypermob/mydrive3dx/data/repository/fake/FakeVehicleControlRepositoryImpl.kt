package com.hypermob.mydrive3dx.data.repository.fake

import com.hypermob.mydrive3dx.domain.model.*
import com.hypermob.mydrive3dx.domain.repository.VehicleControlRepository
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import javax.inject.Inject
import javax.inject.Singleton

/**
 * Fake Vehicle Control Repository Implementation
 *
 * API가 준비되기 전 테스트용 Repository
 */
@Singleton
class FakeVehicleControlRepositoryImpl @Inject constructor() : VehicleControlRepository {

    // 인메모리 차량 상태
    private var currentVehicleControl = DummyData.dummyVehicleControl.copy()

    override suspend fun getVehicleControl(rentalId: String): Flow<Result<VehicleControl>> = flow {
        emit(Result.Loading)
        delay(500)
        emit(Result.Success(currentVehicleControl))
    }

    override suspend fun lockDoors(rentalId: String): Flow<Result<ControlCommandResponse>> = flow {
        emit(Result.Loading)
        delay(300)

        currentVehicleControl = currentVehicleControl.copy(
            doors = DoorStatus(
                frontLeft = true,
                frontRight = true,
                rearLeft = true,
                rearRight = true,
                trunk = true
            )
        )

        emit(Result.Success(DummyData.createDummyControlResponse(message = "문 잠금 성공")))
    }

    override suspend fun unlockDoors(rentalId: String): Flow<Result<ControlCommandResponse>> = flow {
        emit(Result.Loading)
        delay(300)

        currentVehicleControl = currentVehicleControl.copy(
            doors = DoorStatus(
                frontLeft = false,
                frontRight = false,
                rearLeft = false,
                rearRight = false,
                trunk = false
            )
        )

        emit(Result.Success(DummyData.createDummyControlResponse(message = "문 잠금 해제 성공")))
    }

    override suspend fun startEngine(rentalId: String): Flow<Result<ControlCommandResponse>> = flow {
        emit(Result.Loading)
        delay(500)

        currentVehicleControl = currentVehicleControl.copy(
            engine = currentVehicleControl.engine.copy(isRunning = true, rpm = 800)
        )

        emit(Result.Success(DummyData.createDummyControlResponse(message = "시동 켜기 성공")))
    }

    override suspend fun stopEngine(rentalId: String): Flow<Result<ControlCommandResponse>> = flow {
        emit(Result.Loading)
        delay(500)

        currentVehicleControl = currentVehicleControl.copy(
            engine = currentVehicleControl.engine.copy(isRunning = false, rpm = 0)
        )

        emit(Result.Success(DummyData.createDummyControlResponse(message = "시동 끄기 성공")))
    }

    override suspend fun turnOnClimate(
        rentalId: String,
        temperature: Double
    ): Flow<Result<ControlCommandResponse>> = flow {
        emit(Result.Loading)
        delay(400)

        currentVehicleControl = currentVehicleControl.copy(
            climate = currentVehicleControl.climate.copy(
                isOn = true,
                temperature = temperature,
                fanSpeed = 3
            )
        )

        emit(Result.Success(DummyData.createDummyControlResponse(message = "에어컨 켜기 성공")))
    }

    override suspend fun turnOffClimate(rentalId: String): Flow<Result<ControlCommandResponse>> = flow {
        emit(Result.Loading)
        delay(400)

        currentVehicleControl = currentVehicleControl.copy(
            climate = currentVehicleControl.climate.copy(isOn = false, fanSpeed = 0)
        )

        emit(Result.Success(DummyData.createDummyControlResponse(message = "에어컨 끄기 성공")))
    }

    override suspend fun honkHorn(rentalId: String): Flow<Result<ControlCommandResponse>> = flow {
        emit(Result.Loading)
        delay(200)
        emit(Result.Success(DummyData.createDummyControlResponse(message = "경적 울리기 성공")))
    }

    override suspend fun flashLights(rentalId: String): Flow<Result<ControlCommandResponse>> = flow {
        emit(Result.Loading)
        delay(200)
        emit(Result.Success(DummyData.createDummyControlResponse(message = "라이트 깜빡이기 성공")))
    }

    override suspend fun executeCommand(
        request: ControlCommandRequest
    ): Flow<Result<ControlCommandResponse>> = flow {
        emit(Result.Loading)
        delay(300)
        emit(Result.Success(DummyData.createDummyControlResponse(message = "${request.commandType} 실행 성공")))
    }
}
