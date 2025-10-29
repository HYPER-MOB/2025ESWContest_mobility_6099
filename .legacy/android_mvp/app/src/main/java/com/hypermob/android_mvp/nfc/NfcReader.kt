package com.hypermob.android_mvp.nfc

import android.nfc.NfcAdapter
import android.nfc.Tag
import android.nfc.tech.NfcA

/**
 * NFC Reader Utility
 * Reads UID from ISO14443A NFC tags
 */
object NfcReader {

    /**
     * Extract UID from NFC Tag
     * @return 14-character hex string (7 bytes)
     */
    fun readUid(tag: Tag?): String? {
        if (tag == null) return null

        return try {
            val nfcA = NfcA.get(tag)
            val uid = tag.id

            // Convert byte array to hex string
            uid.joinToString("") { byte ->
                "%02X".format(byte)
            }.uppercase()
        } catch (e: Exception) {
            e.printStackTrace()
            null
        }
    }

    /**
     * Validate NFC UID format
     * Must be 14 hex characters (7 bytes)
     */
    fun isValidUid(uid: String?): Boolean {
        if (uid.isNullOrEmpty()) return false
        return uid.matches(Regex("^[0-9A-Fa-f]{14}$"))
    }
}
