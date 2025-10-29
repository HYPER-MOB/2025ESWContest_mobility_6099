package com.hypermob.mydrive3dx.data.repository.fake

import com.hypermob.mydrive3dx.domain.model.*
import java.time.LocalDateTime
import java.time.format.DateTimeFormatter

/**
 * Dummy Data for Testing
 *
 * API가 준비되기 전 테스트용 더미 데이터
 */
object DummyData {

    private val dateFormatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss")

    // Dummy User
    val dummyUser = User(
        userId = "user001",
        email = "test@hypermob.com",
        name = "김현대",
        phone = "010-1234-5678",
        drivingLicense = "12-34-567890-12",
        profileImage = null,
        createdAt = LocalDateTime.now().minusMonths(6).format(dateFormatter)
    )

    // Dummy Auth Token
    val dummyAuthToken = AuthToken(
        accessToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.dummy_access_token",
        refreshToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.dummy_refresh_token",
        expiresIn = 3600
    )

    // Dummy Vehicles
    val dummyVehicles = listOf(
        Vehicle(
            vehicleId = "vehicle001",
            model = "아이오닉 6",
            manufacturer = "현대",
            year = 2024,
            color = "화이트",
            licensePlate = "123가 4567",
            vehicleType = VehicleType.SEDAN,
            fuelType = FuelType.ELECTRIC,
            transmission = TransmissionType.AUTOMATIC,
            seatingCapacity = 5,
            pricePerHour = 15000.0,
            pricePerDay = 120000.0,
            imageUrl = "https://example.com/images/ioniq6.jpg",
            location = "서울 강남구",
            latitude = 37.4979,
            longitude = 127.0276,
            status = VehicleStatus.AVAILABLE,
            features = listOf("오토파일럿", "전기차", "급속충전", "통풍시트")
        ),
        Vehicle(
            vehicleId = "vehicle002",
            model = "GV80",
            manufacturer = "제네시스",
            year = 2023,
            color = "블랙",
            licensePlate = "456나 7890",
            vehicleType = VehicleType.SUV,
            fuelType = FuelType.GASOLINE,
            transmission = TransmissionType.AUTOMATIC,
            seatingCapacity = 7,
            pricePerHour = 25000.0,
            pricePerDay = 200000.0,
            imageUrl = "https://example.com/images/gv80.jpg",
            location = "서울 서초구",
            latitude = 37.4833,
            longitude = 127.0322,
            status = VehicleStatus.AVAILABLE,
            features = listOf("럭셔리", "7인승", "통풍시트", "HUD")
        ),
        Vehicle(
            vehicleId = "vehicle003",
            model = "카니발",
            manufacturer = "기아",
            year = 2024,
            color = "실버",
            licensePlate = "789다 1234",
            vehicleType = VehicleType.VAN,
            fuelType = FuelType.DIESEL,
            transmission = TransmissionType.AUTOMATIC,
            seatingCapacity = 11,
            pricePerHour = 20000.0,
            pricePerDay = 160000.0,
            imageUrl = "https://example.com/images/carnival.jpg",
            location = "서울 송파구",
            latitude = 37.5145,
            longitude = 127.1059,
            status = VehicleStatus.AVAILABLE,
            features = listOf("11인승", "대형 트렁크", "후방카메라")
        )
    )

    // Dummy Rentals
    val dummyRentals = listOf(
        Rental(
            rentalId = "rental001",
            userId = "user001",
            vehicleId = "vehicle001",
            vehicle = dummyVehicles[0],
            startTime = LocalDateTime.now().minusDays(2).format(dateFormatter),
            endTime = LocalDateTime.now().plusDays(3).format(dateFormatter),
            totalPrice = 600000.0,
            status = RentalStatus.PICKED,
            pickupLocation = "서울 강남구",
            returnLocation = "서울 강남구",
            createdAt = LocalDateTime.now().minusDays(3).format(dateFormatter),
            updatedAt = LocalDateTime.now().minusDays(2).format(dateFormatter)
        ),
        Rental(
            rentalId = "rental002",
            userId = "user001",
            vehicleId = "vehicle002",
            vehicle = dummyVehicles[1],
            startTime = LocalDateTime.now().plusDays(5).format(dateFormatter),
            endTime = LocalDateTime.now().plusDays(7).format(dateFormatter),
            totalPrice = 400000.0,
            status = RentalStatus.APPROVED,
            pickupLocation = "서울 서초구",
            returnLocation = "서울 서초구",
            createdAt = LocalDateTime.now().minusDays(1).format(dateFormatter),
            updatedAt = LocalDateTime.now().minusDays(1).format(dateFormatter)
        ),
        Rental(
            rentalId = "rental003",
            userId = "user001",
            vehicleId = "vehicle003",
            vehicle = dummyVehicles[2],
            startTime = LocalDateTime.now().minusDays(10).format(dateFormatter),
            endTime = LocalDateTime.now().minusDays(8).format(dateFormatter),
            totalPrice = 320000.0,
            status = RentalStatus.RETURNED,
            pickupLocation = "서울 송파구",
            returnLocation = "서울 송파구",
            createdAt = LocalDateTime.now().minusDays(12).format(dateFormatter),
            updatedAt = LocalDateTime.now().minusDays(8).format(dateFormatter)
        )
    )

    // Dummy Vehicle Control
    val dummyVehicleControl = VehicleControl(
        vehicleId = "vehicle001",
        rentalId = "rental001",
        status = VehicleControlStatus(
            isOnline = true,
            batteryLevel = 85,
            fuelLevel = 0,
            odometer = 12345.0
        ),
        doors = DoorStatus(
            frontLeft = true,
            frontRight = true,
            rearLeft = true,
            rearRight = true,
            trunk = true
        ),
        engine = EngineStatus(
            isRunning = false,
            temperature = 25.0,
            rpm = 0
        ),
        climate = ClimateStatus(
            isOn = false,
            temperature = 22.0,
            fanSpeed = 0
        ),
        location = VehicleLocation(
            latitude = 37.4979,
            longitude = 127.0276,
            address = "서울 강남구"
        ),
        lastUpdated = LocalDateTime.now().format(dateFormatter)
    )

    // Dummy Control Command Response
    fun createDummyControlResponse(success: Boolean = true, message: String = "명령이 성공적으로 실행되었습니다"): ControlCommandResponse {
        return ControlCommandResponse(
            success = success,
            message = message,
            updatedControl = dummyVehicleControl
        )
    }

    // Dummy Body Scan
    val dummyBodyScan = BodyScan(
        scanId = "scan001",
        userId = "user001",
        timestamp = LocalDateTime.now().format(dateFormatter),
        sittingHeight = 90.0,
        shoulderWidth = 45.0,
        armLength = 60.0,
        headHeight = 25.0,
        eyeHeight = 115.0,
        legLength = 90.0,
        torsoLength = 65.0,
        weight = 70.0,
        height = 175.0
    )
}
