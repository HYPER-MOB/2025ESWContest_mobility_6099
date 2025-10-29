package com.hypermob.mydrive3dx.di

import com.hypermob.mydrive3dx.data.local.datasource.*
import dagger.Binds
import dagger.Module
import dagger.hilt.InstallIn
import dagger.hilt.components.SingletonComponent
import javax.inject.Singleton

/**
 * Data Source Module
 *
 * 로컬 데이터 소스 의존성 제공
 */
@Module
@InstallIn(SingletonComponent::class)
abstract class DataSourceModule {

    @Binds
    @Singleton
    abstract fun bindUserLocalDataSource(
        userLocalDataSourceImpl: UserLocalDataSourceImpl
    ): UserLocalDataSource

    @Binds
    @Singleton
    abstract fun bindVehicleLocalDataSource(
        vehicleLocalDataSourceImpl: VehicleLocalDataSourceImpl
    ): VehicleLocalDataSource

    @Binds
    @Singleton
    abstract fun bindRentalLocalDataSource(
        rentalLocalDataSourceImpl: RentalLocalDataSourceImpl
    ): RentalLocalDataSource
}
