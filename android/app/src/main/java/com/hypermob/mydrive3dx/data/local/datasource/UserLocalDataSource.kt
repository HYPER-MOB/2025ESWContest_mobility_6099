package com.hypermob.mydrive3dx.data.local.datasource

import com.hypermob.mydrive3dx.data.local.dao.UserDao
import com.hypermob.mydrive3dx.data.local.entity.UserEntity
import com.hypermob.mydrive3dx.data.mapper.toDomain
import com.hypermob.mydrive3dx.data.mapper.toEntity
import com.hypermob.mydrive3dx.domain.model.User
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map
import javax.inject.Inject
import javax.inject.Singleton

/**
 * User Local Data Source
 *
 * 사용자 정보 로컬 데이터 소스 (Room)
 */
interface UserLocalDataSource {
    fun getUserById(userId: String): Flow<User?>
    suspend fun getUserByIdOnce(userId: String): User?
    suspend fun getUserByEmail(email: String): User?
    suspend fun saveUser(user: User)
    suspend fun updateUser(user: User)
    suspend fun deleteUser(userId: String)
    suspend fun clearAllUsers()
}

@Singleton
class UserLocalDataSourceImpl @Inject constructor(
    private val userDao: UserDao
) : UserLocalDataSource {

    override fun getUserById(userId: String): Flow<User?> {
        return userDao.getUserByIdFlow(userId).map { it?.toDomain() }
    }

    override suspend fun getUserByIdOnce(userId: String): User? {
        return userDao.getUserById(userId)?.toDomain()
    }

    override suspend fun getUserByEmail(email: String): User? {
        return userDao.getUserByEmail(email)?.toDomain()
    }

    override suspend fun saveUser(user: User) {
        userDao.insertUser(user.toEntity())
    }

    override suspend fun updateUser(user: User) {
        userDao.updateUser(user.toEntity())
    }

    override suspend fun deleteUser(userId: String) {
        userDao.deleteUserById(userId)
    }

    override suspend fun clearAllUsers() {
        userDao.deleteAllUsers()
    }
}
