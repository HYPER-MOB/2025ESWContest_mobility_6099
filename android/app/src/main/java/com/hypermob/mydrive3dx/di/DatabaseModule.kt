package com.hypermob.mydrive3dx.di

import android.content.Context
import androidx.room.Room
import com.hypermob.mydrive3dx.data.local.MyDriveDatabase
import com.hypermob.mydrive3dx.data.local.dao.RentalDao
import com.hypermob.mydrive3dx.data.local.dao.UserDao
import com.hypermob.mydrive3dx.data.local.dao.VehicleDao
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.android.qualifiers.ApplicationContext
import dagger.hilt.components.SingletonComponent
import javax.inject.Singleton

/**
 * Database Module
 *
 * Room Database 의존성 제공
 */
@Module
@InstallIn(SingletonComponent::class)
object DatabaseModule {

    @Provides
    @Singleton
    fun provideMyDriveDatabase(
        @ApplicationContext context: Context
    ): MyDriveDatabase {
        return Room.databaseBuilder(
            context,
            MyDriveDatabase::class.java,
            MyDriveDatabase.DATABASE_NAME
        )
            .fallbackToDestructiveMigration()
            .build()
    }

    @Provides
    @Singleton
    fun provideUserDao(database: MyDriveDatabase): UserDao {
        return database.userDao()
    }

    @Provides
    @Singleton
    fun provideVehicleDao(database: MyDriveDatabase): VehicleDao {
        return database.vehicleDao()
    }

    @Provides
    @Singleton
    fun provideRentalDao(database: MyDriveDatabase): RentalDao {
        return database.rentalDao()
    }
}
