package com.hypermob.android_mvp.service

import android.nfc.cardemulation.HostApduService
import android.os.Bundle
import android.util.Log
import com.hypermob.android_mvp.util.PreferenceManager

/**
 * NFC Host Card Emulation (HCE) Service
 *
 * Emulates an NFC tag that transmits the user's NFC UID
 * to the car's NFC reader for authentication.
 *
 * Flow:
 * 1. Car NFC reader sends SELECT AID command
 * 2. This service responds with stored nfc_uid
 * 3. Car validates the nfc_uid against server data
 */
class NfcHceService : HostApduService() {

    companion object {
        private const val TAG = "NfcHceService"

        // ISO 7816-4 Status Words
        private val STATUS_SUCCESS = byteArrayOf(0x90.toByte(), 0x00.toByte())
        private val STATUS_FAILED = byteArrayOf(0x6F.toByte(), 0x00.toByte())

        // APDU Command: SELECT AID
        private const val SELECT_APDU_HEADER = "00A40400"

        // Our custom AID (Application ID): F0010203040506
        private val AID = byteArrayOf(
            0xF0.toByte(), 0x01, 0x02, 0x03, 0x04, 0x05, 0x06
        )
    }

    override fun onCreate() {
        super.onCreate()
        Log.d(TAG, "✓ NFC HCE Service created!")
    }

    override fun onDeactivated(reason: Int) {
        val reasonText = when (reason) {
            DEACTIVATION_LINK_LOSS -> "LINK_LOSS"
            DEACTIVATION_DESELECTED -> "DESELECTED"
            else -> "UNKNOWN($reason)"
        }
        Log.d(TAG, "NFC deactivated, reason: $reasonText")
    }

    override fun processCommandApdu(commandApdu: ByteArray, extras: Bundle?): ByteArray {
        Log.d(TAG, "Received APDU: ${commandApdu.toHexString()}")

        // Check if this is a SELECT AID command
        if (isSelectAidApdu(commandApdu)) {
            Log.d(TAG, "SELECT AID command received")

            // Get stored NFC UID
            val nfcUid = PreferenceManager.getNfcUid(this)

            if (nfcUid.isNullOrEmpty()) {
                Log.e(TAG, "NFC UID not found in preferences")
                return STATUS_FAILED
            }

            Log.d(TAG, "Sending NFC UID: $nfcUid")

            // Convert hex string to bytes and append success status
            return hexStringToByteArray(nfcUid) + STATUS_SUCCESS
        }

        // Unknown command
        Log.w(TAG, "Unknown APDU command: ${commandApdu.toHexString()}")
        return STATUS_FAILED
    }

    /**
     * Check if the APDU is a SELECT AID command for our AID
     */
    private fun isSelectAidApdu(apdu: ByteArray): Boolean {
        // SELECT APDU format: 00 A4 04 00 [Lc] [AID] [Le]
        // Minimum length: 5 (header) + AID length
        if (apdu.size < 5 + AID.size) {
            return false
        }

        // Check if it's a SELECT command (00 A4 04 00)
        if (apdu[0] != 0x00.toByte() ||
            apdu[1] != 0xA4.toByte() ||
            apdu[2] != 0x04.toByte() ||
            apdu[3] != 0x00.toByte()
        ) {
            return false
        }

        // Check Lc (length of AID)
        val lc = apdu[4].toInt() and 0xFF
        if (lc != AID.size) {
            return false
        }

        // Check if AID matches
        for (i in AID.indices) {
            if (apdu[5 + i] != AID[i]) {
                return false
            }
        }

        return true
    }

    /**
     * Convert hex string to byte array
     */
    private fun hexStringToByteArray(hex: String): ByteArray {
        val cleanHex = hex.replace(" ", "").replace(":", "")
        val len = cleanHex.length
        val data = ByteArray(len / 2)

        for (i in 0 until len step 2) {
            data[i / 2] = ((Character.digit(cleanHex[i], 16) shl 4) +
                          Character.digit(cleanHex[i + 1], 16)).toByte()
        }

        return data
    }

    /**
     * Convert byte array to hex string for logging
     */
    private fun ByteArray.toHexString(): String {
        return joinToString(" ") { "%02X".format(it) }
    }
}
