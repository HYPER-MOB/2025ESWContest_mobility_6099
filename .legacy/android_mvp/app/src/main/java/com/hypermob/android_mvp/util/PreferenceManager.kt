package com.hypermob.android_mvp.util

import android.content.Context
import android.content.SharedPreferences

/**
 * Shared Preferences Manager for storing user data
 */
class PreferenceManager(context: Context) {

    private val prefs: SharedPreferences = context.getSharedPreferences(
        PREFS_NAME,
        Context.MODE_PRIVATE
    )

    companion object {
        private const val PREFS_NAME = "hypermob_mfa_prefs"
        private const val KEY_USER_ID = "user_id"
        private const val KEY_FACE_ID = "face_id"
        private const val KEY_NFC_UID = "nfc_uid"
        private const val KEY_IS_REGISTERED = "is_registered"

        /**
         * Static method to get NFC UID (for use in Services)
         */
        fun getNfcUid(context: Context): String? {
            val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            return prefs.getString(KEY_NFC_UID, null)
        }

        /**
         * Static method to get User ID (for use in Services)
         */
        fun getUserId(context: Context): String? {
            val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            return prefs.getString(KEY_USER_ID, null)
        }
    }

    var userId: String?
        get() = prefs.getString(KEY_USER_ID, null)
        set(value) = prefs.edit().putString(KEY_USER_ID, value).apply()

    var faceId: String?
        get() = prefs.getString(KEY_FACE_ID, null)
        set(value) = prefs.edit().putString(KEY_FACE_ID, value).apply()

    var nfcUid: String?
        get() = prefs.getString(KEY_NFC_UID, null)
        set(value) = prefs.edit().putString(KEY_NFC_UID, value).apply()

    var isRegistered: Boolean
        get() = prefs.getBoolean(KEY_IS_REGISTERED, false)
        set(value) = prefs.edit().putBoolean(KEY_IS_REGISTERED, value).apply()

    fun clear() {
        prefs.edit().clear().apply()
    }

    fun isFullyRegistered(): Boolean {
        return !userId.isNullOrEmpty() && !faceId.isNullOrEmpty() && !nfcUid.isNullOrEmpty()
    }
}
