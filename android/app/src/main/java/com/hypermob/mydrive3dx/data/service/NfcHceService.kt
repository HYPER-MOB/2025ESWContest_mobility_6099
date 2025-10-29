package com.hypermob.mydrive3dx.data.service

import android.nfc.cardemulation.HostApduService
import android.os.Bundle
import android.util.Log
import com.hypermob.mydrive3dx.data.local.TokenManager
import dagger.hilt.android.AndroidEntryPoint
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.runBlocking
import javax.inject.Inject

/**
 * NFC Host Card Emulation Service
 * android_mvp의 NfcHceService 로직 기반
 *
 * 휴대폰을 NFC 카드처럼 동작하게 하여 차량이 NFC UID를 읽을 수 있도록 합니다.
 * SELECT AID 명령을 받으면 TokenManager에 저장된 nfc_uid를 응답합니다.
 */
@AndroidEntryPoint
class NfcHceService : HostApduService() {

    @Inject
    lateinit var tokenManager: TokenManager

    private val serviceScope = CoroutineScope(SupervisorJob() + Dispatchers.IO)

    companion object {
        private const val TAG = "NfcHceService"
        private const val AID = "F0010203040506"
        private const val SELECT_APDU_HEADER = "00A4040007"
        private val STATUS_SUCCESS = byteArrayOf(0x90.toByte(), 0x00.toByte())
        private val STATUS_FAILED = byteArrayOf(0x6F.toByte(), 0x00.toByte())
    }

    override fun processCommandApdu(commandApdu: ByteArray?, extras: Bundle?): ByteArray {
        if (commandApdu == null) {
            Log.w(TAG, "Received null commandApdu")
            return STATUS_FAILED
        }

        val command = commandApdu.toHexString()
        Log.d(TAG, "Received APDU: $command")

        return when {
            // SELECT AID 명령 처리
            isSelectAidApdu(commandApdu) -> {
                handleSelectAid()
            }
            else -> {
                Log.w(TAG, "Unknown APDU command: $command")
                STATUS_FAILED
            }
        }
    }

    /**
     * SELECT AID 명령 확인
     * android_mvp 로직 기반
     */
    private fun isSelectAidApdu(apdu: ByteArray): Boolean {
        val command = apdu.toHexString()
        return command.startsWith(SELECT_APDU_HEADER + AID)
    }

    /**
     * SELECT AID 명령 처리
     * TokenManager에서 nfc_uid를 읽어 응답
     * android_mvp의 NfcHceService.kt 로직 기반
     */
    private fun handleSelectAid(): ByteArray {
        return try {
            // TokenManager에서 nfc_uid 조회 (동기)
            val nfcUid = runBlocking {
                tokenManager.getNfcUid()
            }

            if (nfcUid.isNullOrEmpty()) {
                Log.w(TAG, "NFC UID not found in TokenManager")
                STATUS_FAILED
            } else {
                Log.d(TAG, "Responding with NFC UID: $nfcUid")
                // nfc_uid를 바이트 배열로 변환 + SUCCESS 상태 코드
                hexStringToByteArray(nfcUid) + STATUS_SUCCESS
            }
        } catch (e: Exception) {
            Log.e(TAG, "Error reading NFC UID", e)
            STATUS_FAILED
        }
    }

    override fun onDeactivated(reason: Int) {
        val reasonStr = when (reason) {
            DEACTIVATION_LINK_LOSS -> "Link Loss"
            DEACTIVATION_DESELECTED -> "Deselected"
            else -> "Unknown"
        }
        Log.d(TAG, "NFC Deactivated: $reasonStr")
    }

    override fun onDestroy() {
        super.onDestroy()
        serviceScope.cancel()
    }

    /**
     * ByteArray를 Hex String으로 변환
     */
    private fun ByteArray.toHexString(): String =
        joinToString("") { "%02X".format(it) }

    /**
     * Hex String을 ByteArray로 변환
     * android_mvp 로직 기반
     */
    private fun hexStringToByteArray(hex: String): ByteArray {
        val cleanHex = hex.replace(" ", "").uppercase()
        return ByteArray(cleanHex.length / 2) { i ->
            cleanHex.substring(i * 2, i * 2 + 2).toInt(16).toByte()
        }
    }
}
