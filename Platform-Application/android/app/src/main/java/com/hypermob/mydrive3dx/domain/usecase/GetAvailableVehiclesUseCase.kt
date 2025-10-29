package com.hypermob.mydrive3dx.domain.usecase

import com.hypermob.mydrive3dx.domain.model.Vehicle
import com.hypermob.mydrive3dx.domain.model.VehicleStatus
import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.model.VehicleType
import com.hypermob.mydrive3dx.domain.model.FuelType
import com.hypermob.mydrive3dx.domain.model.TransmissionType
import com.hypermob.mydrive3dx.domain.repository.VehicleRepository
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import javax.inject.Inject

/**
 * Get Available Vehicles Use Case
 * 대여 가능한 차량 목록 조회
 */
class GetAvailableVehiclesUseCase @Inject constructor(
    private val vehicleRepository: VehicleRepository
) {
    suspend operator fun invoke(): Flow<Result<List<Vehicle>>> {
        // 실제로는 repository에서 가져와야 하지만, 일단 더미 데이터로 구현
        return flow {
            emit(Result.Loading)

            try {
                // TODO: 실제 API 연결시 아래 코드로 변경
                // vehicleRepository.getAvailableVehicles()

                // 시나리오2 테스트용 더미 데이터
                val dummyVehicles = listOf(
                    Vehicle(
                        vehicleId = "vehicle1",
                        model = "아이오닉 5",
                        manufacturer = "현대",
                        year = 2024,
                        color = "화이트",
                        licensePlate = "서울 12가 3456",
                        vehicleType = VehicleType.ELECTRIC,
                        fuelType = FuelType.ELECTRIC,
                        transmission = TransmissionType.AUTOMATIC,
                        seatingCapacity = 5,
                        pricePerHour = 15000.0,
                        pricePerDay = 120000.0,
                        imageUrl = null,
                        location = "강남역 주차장",
                        latitude = 37.497942,
                        longitude = 127.027621,
                        status = VehicleStatus.AVAILABLE,
                        features = listOf("자율주행", "무선충전", "빌트인캠")
                    ),
                    Vehicle(
                        vehicleId = "vehicle2",
                        model = "Model 3",
                        manufacturer = "테슬라",
                        year = 2023,
                        color = "블랙",
                        licensePlate = "서울 34나 5678",
                        vehicleType = VehicleType.ELECTRIC,
                        fuelType = FuelType.ELECTRIC,
                        transmission = TransmissionType.AUTOMATIC,
                        seatingCapacity = 5,
                        pricePerHour = 20000.0,
                        pricePerDay = 150000.0,
                        imageUrl = null,
                        location = "판교역 주차장",
                        latitude = 37.394776,
                        longitude = 127.111047,
                        status = VehicleStatus.AVAILABLE,
                        features = listOf("오토파일럿", "센트리모드", "캠핑모드")
                    ),
                    Vehicle(
                        vehicleId = "vehicle3",
                        model = "GV70",
                        manufacturer = "제네시스",
                        year = 2024,
                        color = "실버",
                        licensePlate = "서울 56다 7890",
                        vehicleType = VehicleType.SUV,
                        fuelType = FuelType.GASOLINE,
                        transmission = TransmissionType.AUTOMATIC,
                        seatingCapacity = 5,
                        pricePerHour = 18000.0,
                        pricePerDay = 140000.0,
                        imageUrl = null,
                        location = "서울역 주차장",
                        latitude = 37.554722,
                        longitude = 126.970833,
                        status = VehicleStatus.AVAILABLE,
                        features = listOf("통풍시트", "마사지시트", "HUD")
                    ),
                    Vehicle(
                        vehicleId = "vehicle4",
                        model = "쏘렌토",
                        manufacturer = "기아",
                        year = 2023,
                        color = "네이비",
                        licensePlate = "서울 78라 1234",
                        vehicleType = VehicleType.SUV,
                        fuelType = FuelType.DIESEL,
                        transmission = TransmissionType.AUTOMATIC,
                        seatingCapacity = 7,
                        pricePerHour = 16000.0,
                        pricePerDay = 130000.0,
                        imageUrl = null,
                        location = "잠실역 주차장",
                        latitude = 37.513311,
                        longitude = 127.100164,
                        status = VehicleStatus.AVAILABLE,
                        features = listOf("7인승", "후방카메라", "크루즈컨트롤")
                    ),
                    Vehicle(
                        vehicleId = "vehicle5",
                        model = "아반떼 N",
                        manufacturer = "현대",
                        year = 2024,
                        color = "블루",
                        licensePlate = "서울 90마 5678",
                        vehicleType = VehicleType.SEDAN,
                        fuelType = FuelType.GASOLINE,
                        transmission = TransmissionType.MANUAL,
                        seatingCapacity = 5,
                        pricePerHour = 12000.0,
                        pricePerDay = 100000.0,
                        imageUrl = null,
                        location = "홍대입구역 주차장",
                        latitude = 37.557192,
                        longitude = 126.925381,
                        status = VehicleStatus.RENTED,  // 대여중
                        features = listOf("스포츠모드", "런치컨트롤", "전자식LSD")
                    )
                )

                emit(Result.Success(dummyVehicles))
            } catch (e: Exception) {
                emit(Result.Error(e))
            }
        }
    }
}