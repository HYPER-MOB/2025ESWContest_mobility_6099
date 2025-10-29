package com.hypermob.mydrive3dx.data.repository

import com.hypermob.mydrive3dx.data.remote.api.UserProfileApi
import com.hypermob.mydrive3dx.domain.model.UserProfile
import com.hypermob.mydrive3dx.domain.repository.UserProfileRepository
import javax.inject.Inject

/**
 * User Profile Repository Implementation
 * 사용자 프로필 관련 데이터 저장소 구현
 */
class UserProfileRepositoryImpl @Inject constructor(
    private val api: UserProfileApi
) : UserProfileRepository {

    override suspend fun getProfile(userId: String): UserProfile {
        return api.getProfile(userId)
    }

    override suspend fun updateProfile(userId: String, profile: UserProfile): UserProfile {
        return api.updateProfile(userId, profile)
    }

    override suspend fun deleteProfile(userId: String) {
        return api.deleteProfile(userId)
    }
}
