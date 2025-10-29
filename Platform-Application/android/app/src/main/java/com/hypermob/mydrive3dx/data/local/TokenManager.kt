package com.hypermob.mydrive3dx.data.local

import android.content.Context
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.longPreferencesKey
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import com.hypermob.mydrive3dx.domain.model.AuthToken
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.flow.map
import javax.inject.Inject
import javax.inject.Singleton

/**
 * Token Manager
 *
 * JWT 토큰을 DataStore에 안전하게 저장/관리
 */
@Singleton
class TokenManager @Inject constructor(
    @ApplicationContext private val context: Context
) {
    private val Context.dataStore: DataStore<Preferences> by preferencesDataStore(name = "auth_prefs")

    companion object {
        private val ACCESS_TOKEN_KEY = stringPreferencesKey("access_token")
        private val REFRESH_TOKEN_KEY = stringPreferencesKey("refresh_token")
        private val EXPIRES_IN_KEY = longPreferencesKey("expires_in")

        // MFA 관련 키 (android_mvp 호환)
        private val USER_ID_KEY = stringPreferencesKey("user_id")
        private val FACE_ID_KEY = stringPreferencesKey("face_id")
        private val NFC_UID_KEY = stringPreferencesKey("nfc_uid")
    }

    /**
     * 토큰 저장
     */
    suspend fun saveToken(token: AuthToken) {
        context.dataStore.edit { preferences ->
            preferences[ACCESS_TOKEN_KEY] = token.accessToken
            preferences[REFRESH_TOKEN_KEY] = token.refreshToken
            preferences[EXPIRES_IN_KEY] = token.expiresIn
        }
    }

    /**
     * 토큰 조회 (Flow)
     */
    fun getTokenFlow(): Flow<AuthToken?> {
        return context.dataStore.data.map { preferences ->
            val accessToken = preferences[ACCESS_TOKEN_KEY]
            val refreshToken = preferences[REFRESH_TOKEN_KEY]
            val expiresIn = preferences[EXPIRES_IN_KEY]

            if (accessToken != null && refreshToken != null && expiresIn != null) {
                AuthToken(accessToken, refreshToken, expiresIn)
            } else {
                null
            }
        }
    }

    /**
     * 토큰 조회 (suspend)
     */
    suspend fun getToken(): AuthToken? {
        return getTokenFlow().first()
    }

    /**
     * Access Token 조회
     */
    suspend fun getAccessToken(): String? {
        return context.dataStore.data.map { preferences ->
            preferences[ACCESS_TOKEN_KEY]
        }.first()
    }

    /**
     * Refresh Token 조회
     */
    suspend fun getRefreshToken(): String? {
        return context.dataStore.data.map { preferences ->
            preferences[REFRESH_TOKEN_KEY]
        }.first()
    }

    /**
     * 토큰 삭제
     */
    suspend fun clearToken() {
        context.dataStore.edit { preferences ->
            preferences.remove(ACCESS_TOKEN_KEY)
            preferences.remove(REFRESH_TOKEN_KEY)
            preferences.remove(EXPIRES_IN_KEY)
        }
    }

    /**
     * 로그인 상태 확인
     */
    fun isLoggedIn(): Flow<Boolean> {
        return context.dataStore.data.map { preferences ->
            preferences[ACCESS_TOKEN_KEY] != null
        }
    }

    // ===== MFA 관련 메서드 (android_mvp 호환) =====

    /**
     * User ID 저장 (회원가입 후)
     */
    suspend fun saveUserId(userId: String) {
        context.dataStore.edit { preferences ->
            preferences[USER_ID_KEY] = userId
        }
    }

    /**
     * MFA 등록 정보 저장
     * 얼굴 등록 후 서버로부터 받은 user_id, face_id, nfc_uid 저장
     */
    suspend fun saveMfaInfo(userId: String, faceId: String, nfcUid: String) {
        context.dataStore.edit { preferences ->
            preferences[USER_ID_KEY] = userId
            preferences[FACE_ID_KEY] = faceId
            preferences[NFC_UID_KEY] = nfcUid
        }
    }

    /**
     * User ID 조회
     */
    suspend fun getUserId(): String? {
        return context.dataStore.data.map { preferences ->
            preferences[USER_ID_KEY]
        }.first()
    }

    /**
     * Face ID 조회
     */
    suspend fun getFaceId(): String? {
        return context.dataStore.data.map { preferences ->
            preferences[FACE_ID_KEY]
        }.first()
    }

    /**
     * NFC UID 조회
     */
    suspend fun getNfcUid(): String? {
        return context.dataStore.data.map { preferences ->
            preferences[NFC_UID_KEY]
        }.first()
    }

    /**
     * NFC UID 조회 (Flow)
     * NfcHceService에서 사용
     */
    fun getNfcUidFlow(): Flow<String?> {
        return context.dataStore.data.map { preferences ->
            preferences[NFC_UID_KEY]
        }
    }

    /**
     * MFA 정보 삭제
     */
    suspend fun clearMfaInfo() {
        context.dataStore.edit { preferences ->
            preferences.remove(USER_ID_KEY)
            preferences.remove(FACE_ID_KEY)
            preferences.remove(NFC_UID_KEY)
        }
    }
}
