package com.hypermob.mydrive3dx.domain.repository

import com.hypermob.mydrive3dx.domain.model.UserProfile

/**
 * User Profile Repository Interface
 * 사용자 프로필 관련 데이터 저장소 인터페이스
 */
interface UserProfileRepository {
    suspend fun getProfile(userId: String): UserProfile
    suspend fun updateProfile(userId: String, profile: UserProfile): UserProfile
    suspend fun deleteProfile(userId: String)
}
