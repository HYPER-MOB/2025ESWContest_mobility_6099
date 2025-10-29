package com.hypermob.mydrive3dx.di

import com.hypermob.mydrive3dx.BuildConfig
import com.hypermob.mydrive3dx.data.local.TokenManager
import com.hypermob.mydrive3dx.data.repository.AuthRepositoryImpl
import com.hypermob.mydrive3dx.data.repository.BodyScanRepositoryImpl
import com.hypermob.mydrive3dx.data.repository.RentalRepositoryImpl
import com.hypermob.mydrive3dx.data.repository.VehicleControlRepositoryImpl
import com.hypermob.mydrive3dx.data.repository.fake.FakeAuthRepositoryImpl
import com.hypermob.mydrive3dx.data.repository.fake.FakeBodyScanRepositoryImpl
import com.hypermob.mydrive3dx.data.repository.fake.FakeRentalRepositoryImpl
import com.hypermob.mydrive3dx.data.repository.fake.FakeVehicleControlRepositoryImpl
import com.hypermob.mydrive3dx.domain.repository.AuthRepository
import com.hypermob.mydrive3dx.domain.repository.BodyScanRepository
import com.hypermob.mydrive3dx.domain.repository.RentalRepository
import com.hypermob.mydrive3dx.domain.repository.VehicleControlRepository
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.components.SingletonComponent
import javax.inject.Singleton

/**
 * Repository Module
 *
 * Repository 인터페이스와 구현체 바인딩
 * Debug 모드에서는 Fake Repository 사용
 */
@Module
@InstallIn(SingletonComponent::class)
object RepositoryModule {

    @Provides
    @Singleton
    fun provideAuthRepository(
        tokenManager: TokenManager,
        realImpl: AuthRepositoryImpl,
        fakeImpl: FakeAuthRepositoryImpl
    ): AuthRepository {
        // Use real implementation for server testing
        return realImpl
    }

    @Provides
    @Singleton
    fun provideRentalRepository(
        realImpl: RentalRepositoryImpl,
        fakeImpl: FakeRentalRepositoryImpl
    ): RentalRepository {
        // Use real implementation for server testing
        return realImpl
    }

    @Provides
    @Singleton
    fun provideVehicleControlRepository(
        realImpl: VehicleControlRepositoryImpl,
        fakeImpl: FakeVehicleControlRepositoryImpl
    ): VehicleControlRepository {
        // Use real implementation for server testing
        return realImpl
    }

    @Provides
    @Singleton
    fun provideBodyScanRepository(
        realImpl: BodyScanRepositoryImpl,
        fakeImpl: FakeBodyScanRepositoryImpl
    ): BodyScanRepository {
        // Use real implementation for server testing
        return realImpl
    }

    // === 새로 추가된 Repository ===

    @Provides
    @Singleton
    fun provideVehicleRepository(
        impl: com.hypermob.mydrive3dx.data.repository.VehicleRepositoryImpl
    ): com.hypermob.mydrive3dx.domain.repository.VehicleRepository = impl

    @Provides
    @Singleton
    fun provideBookingRepository(
        impl: com.hypermob.mydrive3dx.data.repository.BookingRepositoryImpl
    ): com.hypermob.mydrive3dx.domain.repository.BookingRepository = impl

    @Provides
    @Singleton
    fun provideUserProfileRepository(
        impl: com.hypermob.mydrive3dx.data.repository.UserProfileRepositoryImpl
    ): com.hypermob.mydrive3dx.domain.repository.UserProfileRepository = impl

    @Provides
    @Singleton
    fun provideMfaRepository(
        impl: com.hypermob.mydrive3dx.data.repository.MfaRepositoryImpl
    ): com.hypermob.mydrive3dx.domain.repository.MfaRepository = impl
}
