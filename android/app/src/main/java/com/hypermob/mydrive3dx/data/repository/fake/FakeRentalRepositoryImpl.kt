package com.hypermob.mydrive3dx.data.repository.fake

import com.hypermob.mydrive3dx.domain.model.*
import com.hypermob.mydrive3dx.domain.repository.RentalRepository
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import java.time.LocalDateTime
import java.time.format.DateTimeFormatter
import javax.inject.Inject
import javax.inject.Singleton

/**
 * Fake Rental Repository Implementation
 *
 * API가 준비되기 전 테스트용 Repository
 */
@Singleton
class FakeRentalRepositoryImpl @Inject constructor() : RentalRepository {

    private val dateFormatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss")

    // 인메모리 저장소
    private val rentals = DummyData.dummyRentals.toMutableList()
    private val vehicles = DummyData.dummyVehicles.toMutableList()

    override suspend fun getActiveRentals(): Flow<Result<List<Rental>>> = flow {
        emit(Result.Loading)
        delay(500)

        val activeRentals = rentals.filter {
            it.status == RentalStatus.APPROVED || it.status == RentalStatus.PICKED
        }
        emit(Result.Success(activeRentals))
    }

    override suspend fun getAllRentals(
        status: RentalStatus?,
        page: Int,
        pageSize: Int
    ): Flow<Result<List<Rental>>> = flow {
        emit(Result.Loading)
        delay(500)

        val filtered = if (status != null) {
            rentals.filter { it.status == status }
        } else {
            rentals
        }

        emit(Result.Success(filtered))
    }

    override suspend fun getRentalById(rentalId: String): Flow<Result<Rental>> = flow {
        emit(Result.Loading)
        delay(300)

        val rental = rentals.find { it.rentalId == rentalId }
        if (rental != null) {
            emit(Result.Success(rental))
        } else {
            emit(Result.Error(Exception("대여 정보를 찾을 수 없습니다")))
        }
    }

    override suspend fun createRental(request: RentalRequest): Flow<Result<Rental>> = flow {
        emit(Result.Loading)
        delay(500)

        val vehicle = vehicles.find { it.vehicleId == request.vehicleId }
        if (vehicle == null) {
            emit(Result.Error(Exception("차량을 찾을 수 없습니다")))
            return@flow
        }

        val newRental = Rental(
            rentalId = "rental_${System.currentTimeMillis()}",
            userId = "user001",
            vehicleId = request.vehicleId,
            vehicle = vehicle,
            startTime = request.startTime,
            endTime = request.endTime,
            totalPrice = vehicle.pricePerDay * 2, // 임시 계산
            status = RentalStatus.REQUESTED,
            pickupLocation = vehicle.location,
            returnLocation = vehicle.location,
            createdAt = LocalDateTime.now().format(dateFormatter),
            updatedAt = LocalDateTime.now().format(dateFormatter)
        )

        rentals.add(newRental)
        emit(Result.Success(newRental))
    }

    override suspend fun cancelRental(rentalId: String): Flow<Result<Unit>> = flow {
        emit(Result.Loading)
        delay(300)

        val rental = rentals.find { it.rentalId == rentalId }
        if (rental != null) {
            val index = rentals.indexOf(rental)
            rentals[index] = rental.copy(status = RentalStatus.CANCELED)
            emit(Result.Success(Unit))
        } else {
            emit(Result.Error(Exception("대여 정보를 찾을 수 없습니다")))
        }
    }

    override suspend fun pickupVehicle(rentalId: String): Flow<Result<Rental>> = flow {
        emit(Result.Loading)
        delay(500)

        val rental = rentals.find { it.rentalId == rentalId }
        if (rental != null) {
            val index = rentals.indexOf(rental)
            val updatedRental = rental.copy(status = RentalStatus.PICKED)
            rentals[index] = updatedRental
            emit(Result.Success(updatedRental))
        } else {
            emit(Result.Error(Exception("대여 정보를 찾을 수 없습니다")))
        }
    }

    override suspend fun returnVehicle(rentalId: String): Flow<Result<Rental>> = flow {
        emit(Result.Loading)
        delay(500)

        val rental = rentals.find { it.rentalId == rentalId }
        if (rental != null) {
            val index = rentals.indexOf(rental)
            val updatedRental = rental.copy(status = RentalStatus.RETURNED)
            rentals[index] = updatedRental
            emit(Result.Success(updatedRental))
        } else {
            emit(Result.Error(Exception("대여 정보를 찾을 수 없습니다")))
        }
    }

    override suspend fun getAvailableVehicles(): Flow<Result<List<Vehicle>>> = flow {
        emit(Result.Loading)
        delay(500)

        val availableVehicles = vehicles.filter { it.status == VehicleStatus.AVAILABLE }
        emit(Result.Success(availableVehicles))
    }
}
