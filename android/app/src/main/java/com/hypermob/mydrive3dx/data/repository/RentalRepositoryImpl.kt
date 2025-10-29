package com.hypermob.mydrive3dx.data.repository

import com.hypermob.mydrive3dx.data.local.datasource.RentalLocalDataSource
import com.hypermob.mydrive3dx.data.local.datasource.VehicleLocalDataSource
import com.hypermob.mydrive3dx.data.mapper.toDomain
import com.hypermob.mydrive3dx.data.mapper.toDto
import com.hypermob.mydrive3dx.data.remote.api.RentalApi
import com.hypermob.mydrive3dx.domain.model.*
import com.hypermob.mydrive3dx.domain.repository.RentalRepository
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.flow.firstOrNull
import javax.inject.Inject
import javax.inject.Singleton

/**
 * Rental Repository Implementation
 *
 * RentalRepository 인터페이스 구현체
 * 오프라인 캐싱 지원
 */
@Singleton
class RentalRepositoryImpl @Inject constructor(
    private val rentalApi: RentalApi,
    private val rentalLocalDataSource: RentalLocalDataSource,
    private val vehicleLocalDataSource: VehicleLocalDataSource
) : RentalRepository {

    override suspend fun getActiveRentals(): Flow<Result<List<Rental>>> = flow {
        emit(Result.Loading)
        try {
            // Try to fetch from remote
            val response = rentalApi.getActiveRentals()
            if (response.success == true && response.data != null) {
                val rentals = response.data.map { it.toDomain() }
                // Cache the rentals
                rentalLocalDataSource.saveRentals(rentals)
                emit(Result.Success(rentals))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to get active rentals")))
            }
        } catch (e: Exception) {
            // If remote fails, try to get from cache
            try {
                val cachedRentals = rentalLocalDataSource.getRentalsByStatus(
                    listOf(RentalStatus.APPROVED, RentalStatus.PICKED)
                ).firstOrNull()
                if (cachedRentals != null && cachedRentals.isNotEmpty()) {
                    emit(Result.Success(cachedRentals))
                } else {
                    emit(Result.Error(e))
                }
            } catch (cacheError: Exception) {
                emit(Result.Error(e))
            }
        }
    }

    override suspend fun getAllRentals(
        status: RentalStatus?,
        page: Int,
        pageSize: Int
    ): Flow<Result<List<Rental>>> = flow {
        emit(Result.Loading)
        try {
            val statusString = status?.name?.lowercase()
            // TODO: Get actual user ID from auth repository
            val userId = "user001" // Temporary hardcoded value for testing
            val response = rentalApi.getAllRentals(
                userId = userId,
                status = statusString,
                page = page,
                pageSize = pageSize
            )
            if (response.success == true && response.data != null) {
                val rentals = response.data.map { it.toDomain() }
                emit(Result.Success(rentals))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to get rentals")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun getRentalById(rentalId: String): Flow<Result<Rental>> = flow {
        emit(Result.Loading)
        try {
            // Try to fetch from remote
            val response = rentalApi.getRentalById(rentalId)
            if (response.success == true && response.data != null) {
                val rental = response.data.toDomain()
                // Cache the rental
                rentalLocalDataSource.saveRental(rental)
                emit(Result.Success(rental))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to get rental")))
            }
        } catch (e: Exception) {
            // If remote fails, try to get from cache
            try {
                val cachedRental = rentalLocalDataSource.getRentalByIdOnce(rentalId)
                if (cachedRental != null) {
                    emit(Result.Success(cachedRental))
                } else {
                    emit(Result.Error(e))
                }
            } catch (cacheError: Exception) {
                emit(Result.Error(e))
            }
        }
    }

    override suspend fun createRental(request: RentalRequest): Flow<Result<Rental>> = flow {
        emit(Result.Loading)
        try {
            // TODO: Get actual user ID from auth repository or shared preferences
            val userId = "user001" // Temporary hardcoded value for testing
            val response = rentalApi.createRental(request.toDto(userId))
            if (response.success == true && response.data != null) {
                val rental = response.data.toDomain()
                emit(Result.Success(rental))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to create rental")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun cancelRental(rentalId: String): Flow<Result<Unit>> = flow {
        emit(Result.Loading)
        try {
            val response = rentalApi.cancelRental(rentalId)
            if (response.success == true) {
                emit(Result.Success(Unit))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to cancel rental")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun pickupVehicle(rentalId: String): Flow<Result<Rental>> = flow {
        emit(Result.Loading)
        try {
            val response = rentalApi.pickupVehicle(rentalId)
            if (response.success == true && response.data != null) {
                val rental = response.data.toDomain()
                emit(Result.Success(rental))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to pickup vehicle")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun returnVehicle(rentalId: String): Flow<Result<Rental>> = flow {
        emit(Result.Loading)
        try {
            val response = rentalApi.returnVehicle(rentalId)
            if (response.success == true && response.data != null) {
                val rental = response.data.toDomain()
                emit(Result.Success(rental))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to return vehicle")))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }

    override suspend fun getAvailableVehicles(): Flow<Result<List<Vehicle>>> = flow {
        emit(Result.Loading)
        try {
            // Try to fetch from remote
            val response = rentalApi.getAvailableVehicles()
            if (response.success == true && response.data != null) {
                // Extract vehicles list from nested data structure
                val vehicles = response.data.vehicles.map { it.toDomain() }
                // Cache the vehicles
                vehicleLocalDataSource.saveVehicles(vehicles)
                emit(Result.Success(vehicles))
            } else {
                emit(Result.Error(Exception(response.message ?: "Failed to get vehicles")))
            }
        } catch (e: Exception) {
            // If remote fails, try to get from cache
            try {
                val cachedVehicles = vehicleLocalDataSource.getVehiclesByStatus(VehicleStatus.AVAILABLE).firstOrNull()
                if (cachedVehicles != null && cachedVehicles.isNotEmpty()) {
                    emit(Result.Success(cachedVehicles))
                } else {
                    emit(Result.Error(e))
                }
            } catch (cacheError: Exception) {
                emit(Result.Error(e))
            }
        }
    }
}
