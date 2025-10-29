package com.hypermob.mydrive3dx.data.local

import androidx.room.Database
import androidx.room.RoomDatabase
import com.hypermob.mydrive3dx.data.local.dao.RentalDao
import com.hypermob.mydrive3dx.data.local.dao.UserDao
import com.hypermob.mydrive3dx.data.local.dao.VehicleDao
import com.hypermob.mydrive3dx.data.local.entity.RentalEntity
import com.hypermob.mydrive3dx.data.local.entity.UserEntity
import com.hypermob.mydrive3dx.data.local.entity.VehicleEntity

/**
 * MyDrive Room Database
 *
 * 오프라인 캐싱을 위한 로컬 데이터베이스
 */
@Database(
    entities = [
        UserEntity::class,
        VehicleEntity::class,
        RentalEntity::class
    ],
    version = 1,
    exportSchema = false
)
abstract class MyDriveDatabase : RoomDatabase() {

    abstract fun userDao(): UserDao
    abstract fun vehicleDao(): VehicleDao
    abstract fun rentalDao(): RentalDao

    companion object {
        const val DATABASE_NAME = "mydrive_database"
    }
}
