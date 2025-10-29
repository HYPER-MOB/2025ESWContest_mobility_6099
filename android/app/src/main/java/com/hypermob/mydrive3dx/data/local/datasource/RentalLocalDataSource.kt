package com.hypermob.mydrive3dx.data.local.datasource

import com.hypermob.mydrive3dx.data.local.dao.RentalDao
import com.hypermob.mydrive3dx.data.mapper.toDomain
import com.hypermob.mydrive3dx.data.mapper.toEntity
import com.hypermob.mydrive3dx.domain.model.Rental
import com.hypermob.mydrive3dx.domain.model.RentalStatus
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map
import javax.inject.Inject
import javax.inject.Singleton

/**
 * Rental Local Data Source
 *
 * 대여 정보 로컬 데이터 소스 (Room)
 */
interface RentalLocalDataSource {
    fun getRentalsByUserId(userId: String): Flow<List<Rental>>
    fun getRentalById(rentalId: String): Flow<Rental?>
    suspend fun getRentalByIdOnce(rentalId: String): Rental?
    fun getRentalsByStatus(statuses: List<RentalStatus>): Flow<List<Rental>>
    suspend fun saveRental(rental: Rental)
    suspend fun saveRentals(rentals: List<Rental>)
    suspend fun updateRental(rental: Rental)
    suspend fun deleteRental(rentalId: String)
    suspend fun clearAllRentals()
    suspend fun clearOldRentals(timestamp: Long)
}

@Singleton
class RentalLocalDataSourceImpl @Inject constructor(
    private val rentalDao: RentalDao
) : RentalLocalDataSource {

    override fun getRentalsByUserId(userId: String): Flow<List<Rental>> {
        return rentalDao.getRentalsByUserId(userId).map { entities ->
            entities.map { it.toDomain() }
        }
    }

    override fun getRentalById(rentalId: String): Flow<Rental?> {
        return rentalDao.getRentalByIdFlow(rentalId).map { it?.toDomain() }
    }

    override suspend fun getRentalByIdOnce(rentalId: String): Rental? {
        return rentalDao.getRentalById(rentalId)?.toDomain()
    }

    override fun getRentalsByStatus(statuses: List<RentalStatus>): Flow<List<Rental>> {
        val statusNames = statuses.map { it.name }
        return rentalDao.getRentalsByStatus(statusNames).map { entities ->
            entities.map { it.toDomain() }
        }
    }

    override suspend fun saveRental(rental: Rental) {
        rentalDao.insertRental(rental.toEntity())
    }

    override suspend fun saveRentals(rentals: List<Rental>) {
        rentalDao.insertRentals(rentals.map { it.toEntity() })
    }

    override suspend fun updateRental(rental: Rental) {
        rentalDao.updateRental(rental.toEntity())
    }

    override suspend fun deleteRental(rentalId: String) {
        rentalDao.deleteRentalById(rentalId)
    }

    override suspend fun clearAllRentals() {
        rentalDao.deleteAllRentals()
    }

    override suspend fun clearOldRentals(timestamp: Long) {
        rentalDao.deleteOldRentals(timestamp)
    }
}
