package com.hypermob.mydrive3dx.di

import com.hypermob.mydrive3dx.BuildConfig
import com.hypermob.mydrive3dx.data.local.TokenManager
import com.hypermob.mydrive3dx.data.remote.api.AuthApi
import com.hypermob.mydrive3dx.data.remote.api.BodyScanApi
import com.hypermob.mydrive3dx.data.remote.api.RentalApi
import com.hypermob.mydrive3dx.data.remote.api.VehicleControlApi
import com.jakewharton.retrofit2.converter.kotlinx.serialization.asConverterFactory
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.components.SingletonComponent
import kotlinx.coroutines.runBlocking
import kotlinx.serialization.json.Json
import okhttp3.Interceptor
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.OkHttpClient
import okhttp3.logging.HttpLoggingInterceptor
import retrofit2.Retrofit
import java.util.concurrent.TimeUnit
import javax.inject.Singleton

/**
 * Network Module
 *
 * Retrofit, OkHttp, API 인스턴스 제공
 */
@Module
@InstallIn(SingletonComponent::class)
object NetworkModule {

    private const val BASE_URL = "https://dfhab2ellk.execute-api.ap-northeast-2.amazonaws.com/v1/" // TODO: 실제 API URL로 변경

    @Provides
    @Singleton
    fun provideJson(): Json = Json {
        ignoreUnknownKeys = true
        coerceInputValues = true
        isLenient = true
    }

    @Provides
    @Singleton
    fun provideAuthInterceptor(tokenManager: TokenManager): Interceptor {
        return Interceptor { chain ->
            val token = runBlocking { tokenManager.getAccessToken() }
            val request = chain.request().newBuilder()

            if (token != null) {
                request.addHeader("Authorization", "Bearer $token")
            }

            chain.proceed(request.build())
        }
    }

    @Provides
    @Singleton
    fun provideLoggingInterceptor(): HttpLoggingInterceptor {
        return HttpLoggingInterceptor().apply {
            level = if (BuildConfig.DEBUG) {
                HttpLoggingInterceptor.Level.BODY
            } else {
                HttpLoggingInterceptor.Level.NONE
            }
        }
    }

    @Provides
    @Singleton
    fun provideOkHttpClient(
        authInterceptor: Interceptor,
        loggingInterceptor: HttpLoggingInterceptor
    ): OkHttpClient {
        return OkHttpClient.Builder()
            .addInterceptor(authInterceptor)
            .addInterceptor(loggingInterceptor)
            .connectTimeout(30, TimeUnit.SECONDS)
            .readTimeout(30, TimeUnit.SECONDS)
            .writeTimeout(30, TimeUnit.SECONDS)
            .build()
    }

    @Provides
    @Singleton
    fun provideRetrofit(
        okHttpClient: OkHttpClient,
        json: Json
    ): Retrofit {
        val contentType = "application/json".toMediaType()
        return Retrofit.Builder()
            .baseUrl(BASE_URL)
            .client(okHttpClient)
            .addConverterFactory(json.asConverterFactory(contentType))
            .build()
    }

    @Provides
    @Singleton
    fun provideAuthApi(retrofit: Retrofit): AuthApi {
        return retrofit.create(AuthApi::class.java)
    }

    @Provides
    @Singleton
    fun provideRentalApi(retrofit: Retrofit): RentalApi {
        return retrofit.create(RentalApi::class.java)
    }

    @Provides
    @Singleton
    fun provideVehicleControlApi(retrofit: Retrofit): VehicleControlApi {
        return retrofit.create(VehicleControlApi::class.java)
    }

    @Provides
    @Singleton
    fun provideBodyScanApi(retrofit: Retrofit): BodyScanApi {
        return retrofit.create(BodyScanApi::class.java)
    }

    // === 새로 추가된 API 인터페이스 ===

    @Provides
    @Singleton
    fun provideVehicleApi(retrofit: Retrofit): com.hypermob.mydrive3dx.data.remote.api.VehicleApi {
        return retrofit.create(com.hypermob.mydrive3dx.data.remote.api.VehicleApi::class.java)
    }

    @Provides
    @Singleton
    fun provideBookingApi(retrofit: Retrofit): com.hypermob.mydrive3dx.data.remote.api.BookingApi {
        return retrofit.create(com.hypermob.mydrive3dx.data.remote.api.BookingApi::class.java)
    }

    @Provides
    @Singleton
    fun provideUserProfileApi(retrofit: Retrofit): com.hypermob.mydrive3dx.data.remote.api.UserProfileApi {
        return retrofit.create(com.hypermob.mydrive3dx.data.remote.api.UserProfileApi::class.java)
    }

    @Provides
    @Singleton
    fun provideMfaApi(retrofit: Retrofit): com.hypermob.mydrive3dx.data.remote.api.MfaApi {
        return retrofit.create(com.hypermob.mydrive3dx.data.remote.api.MfaApi::class.java)
    }
}
