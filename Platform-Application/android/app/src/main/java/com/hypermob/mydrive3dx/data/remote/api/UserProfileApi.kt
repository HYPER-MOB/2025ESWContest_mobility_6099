package com.hypermob.mydrive3dx.data.remote.api

import com.hypermob.mydrive3dx.domain.model.UserProfile
import retrofit2.http.*

/**
 * User Profile API Interface
 * 사용자 프로필 관련 API
 */
interface UserProfileApi {

    /**
     * GET /users/{user_id}/profile
     * 프로필 조회
     */
    @GET("users/{user_id}/profile")
    suspend fun getProfile(
        @Path("user_id") userId: String
    ): UserProfile

    /**
     * PUT /users/{user_id}/profile
     * 프로필 생성/업데이트
     */
    @PUT("users/{user_id}/profile")
    suspend fun updateProfile(
        @Path("user_id") userId: String,
        @Body profile: UserProfile
    ): UserProfile

    /**
     * DELETE /users/{user_id}/profile
     * 프로필 삭제
     */
    @DELETE("users/{user_id}/profile")
    suspend fun deleteProfile(
        @Path("user_id") userId: String
    )
}
