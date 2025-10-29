package com.hypermob.mydrive3dx.data.hardware

import android.app.Activity
import android.content.Context
import android.nfc.NfcAdapter
import android.nfc.Tag
import android.nfc.tech.IsoDep
import android.nfc.tech.MifareClassic
import android.nfc.tech.MifareUltralight
import android.nfc.tech.Ndef
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.callbackFlow
import javax.inject.Inject
import javax.inject.Singleton

/**
 * NFC Manager
 *
 * Android NFC API를 사용한 NFC 태그 읽기 관리자
 */
@Singleton
class NfcManager @Inject constructor(
    @ApplicationContext private val context: Context
) {
    private val nfcAdapter: NfcAdapter? = NfcAdapter.getDefaultAdapter(context)

    /**
     * NFC 태그 감지 결과 상태
     */
    sealed class NfcResult {
        object Idle : NfcResult()
        object Scanning : NfcResult()
        data class TagDetected(
            val tagId: String,
            val tagType: String,
            val techList: List<String>
        ) : NfcResult()
        data class Error(val message: String) : NfcResult()
    }

    /**
     * NFC 지원 여부 확인
     */
    fun isNfcSupported(): Boolean {
        return nfcAdapter != null
    }

    /**
     * NFC 활성화 여부 확인
     */
    fun isNfcEnabled(): Boolean {
        return nfcAdapter?.isEnabled == true
    }

    /**
     * 휴대폰의 NFC ID 가져오기
     *
     * Android 10 (API 29) 이상에서는 보안상 제한됨
     * 대신 고유한 디바이스 식별자를 생성하여 사용
     */
    fun getPhoneNfcId(): String? {
        return try {
            // Android 기기의 고유 ID 생성
            val androidId = android.provider.Settings.Secure.getString(
                context.contentResolver,
                android.provider.Settings.Secure.ANDROID_ID
            )

            // NFC 어댑터가 있는 경우, 조합하여 고유 ID 생성
            if (nfcAdapter != null) {
                // SHA-256 해시로 고유 ID 생성
                val input = "NFC_${androidId}_${System.currentTimeMillis() / (1000 * 60 * 60 * 24)}" // 하루 단위
                val bytes = java.security.MessageDigest.getInstance("SHA-256")
                    .digest(input.toByteArray())

                bytes.joinToString("") { "%02x".format(it) }.take(32) // 32자리로 제한
            } else {
                null
            }
        } catch (e: Exception) {
            null
        }
    }

    /**
     * 휴대폰을 NFC 카드로 에뮬레이션하기 위한 고유 ID 생성
     *
     * 실제로는 Host Card Emulation (HCE) AID를 사용하거나,
     * 안전한 엘리먼트의 UID를 사용해야 함
     */
    fun generatePhoneNfcIdentifier(): String {
        val androidId = android.provider.Settings.Secure.getString(
            context.contentResolver,
            android.provider.Settings.Secure.ANDROID_ID
        )

        // 고유하고 재현 가능한 ID 생성
        val input = "MYDRIVE_NFC_${androidId}"
        val bytes = java.security.MessageDigest.getInstance("SHA-256")
            .digest(input.toByteArray())

        // 16진수 문자열로 변환 (NFC 태그 ID 형식과 유사)
        return bytes.joinToString(":") { "%02X".format(it) }.take(23) // AA:BB:CC:DD:EE:FF:GG 형식
    }

    /**
     * NFC Reader Mode 활성화
     *
     * Activity에서 호출하여 NFC 태그를 읽을 준비를 합니다.
     */
    fun enableReaderMode(activity: Activity, onTagDetected: (Tag) -> Unit) {
        if (!isNfcSupported()) {
            return
        }

        val options = android.os.Bundle()
        // 빠른 감지를 위한 설정
        options.putInt(NfcAdapter.EXTRA_READER_PRESENCE_CHECK_DELAY, 250)

        nfcAdapter?.enableReaderMode(
            activity,
            { tag ->
                onTagDetected(tag)
            },
            NfcAdapter.FLAG_READER_NFC_A or
                    NfcAdapter.FLAG_READER_NFC_B or
                    NfcAdapter.FLAG_READER_NFC_F or
                    NfcAdapter.FLAG_READER_NFC_V or
                    NfcAdapter.FLAG_READER_NO_PLATFORM_SOUNDS,
            options
        )
    }

    /**
     * NFC Reader Mode 비활성화
     */
    fun disableReaderMode(activity: Activity) {
        nfcAdapter?.disableReaderMode(activity)
    }

    /**
     * NFC 태그 정보 파싱
     */
    fun parseTag(tag: Tag): NfcResult.TagDetected {
        val tagId = tag.id.joinToString(":") { String.format("%02X", it) }
        val techList = tag.techList.map { it.substringAfterLast(".") }
        val tagType = detectTagType(tag)

        return NfcResult.TagDetected(
            tagId = tagId,
            tagType = tagType,
            techList = techList
        )
    }

    /**
     * 태그 타입 감지
     */
    private fun detectTagType(tag: Tag): String {
        return when {
            tag.techList.contains(IsoDep::class.java.name) -> "ISO-DEP (Smart Card)"
            tag.techList.contains(MifareClassic::class.java.name) -> "MIFARE Classic"
            tag.techList.contains(MifareUltralight::class.java.name) -> "MIFARE Ultralight"
            tag.techList.contains(Ndef::class.java.name) -> "NDEF"
            else -> "Unknown"
        }
    }

    /**
     * 차량 스마트 키 검증
     *
     * 실제 구현에서는 서버와 통신하여 태그 ID를 검증해야 합니다.
     * 여기서는 간단한 로컬 검증만 수행합니다.
     */
    fun verifyVehicleKey(tagId: String, rentalId: String): Boolean {
        // TODO: 실제로는 서버 API를 호출하여 tagId와 rentalId를 검증
        // 지금은 시뮬레이션으로 항상 true 반환
        // 태그 ID가 최소 8자 이상이면 유효한 것으로 간주
        return tagId.replace(":", "").length >= 8
    }

    /**
     * NDEF 메시지 읽기 (선택적)
     *
     * NDEF 포맷의 NFC 태그에서 메시지를 읽습니다.
     */
    fun readNdefMessage(tag: Tag): String? {
        val ndef = Ndef.get(tag) ?: return null
        try {
            ndef.connect()
            val ndefMessage = ndef.ndefMessage
            val records = ndefMessage?.records
            if (records != null && records.isNotEmpty()) {
                val payload = records[0].payload
                // Skip language code in first 3 bytes for text records
                return String(payload.sliceArray(3 until payload.size))
            }
        } catch (e: Exception) {
            return null
        } finally {
            try {
                ndef.close()
            } catch (e: Exception) {
                // Ignore
            }
        }
        return null
    }

    // === android_mvp에서 추가된 MFA 인증 기능 (Flow 기반) ===

    /**
     * NFC Tag Result
     * android_mvp 통합 - Flow 기반 태그 정보
     */
    data class NfcTagResult(
        val uid: String,
        val techList: List<String>,
        val isoDep: IsoDep?
    )

    /**
     * NFC Reader Mode 활성화 (Flow 기반 - android_mvp 통합)
     */
    fun enableReaderMode(activity: Activity): Flow<NfcTagResult> = callbackFlow {
        if (!isNfcSupported() || !isNfcEnabled()) {
            close(Exception("NFC not available or disabled"))
            return@callbackFlow
        }

        val callback = NfcAdapter.ReaderCallback { tag ->
            val result = processTag(tag)
            trySend(result)
        }

        val options = android.os.Bundle().apply {
            putInt(NfcAdapter.EXTRA_READER_PRESENCE_CHECK_DELAY, 250)
        }

        nfcAdapter?.enableReaderMode(
            activity,
            callback,
            NfcAdapter.FLAG_READER_NFC_A or
            NfcAdapter.FLAG_READER_NFC_B or
            NfcAdapter.FLAG_READER_SKIP_NDEF_CHECK,
            options
        )

        awaitClose {
            nfcAdapter?.disableReaderMode(activity)
        }
    }

    /**
     * NFC 태그 처리 (android_mvp 통합)
     */
    private fun processTag(tag: Tag): NfcTagResult {
        val uid = tag.id.toHexString()
        val techList = tag.techList.toList()
        val isoDep = IsoDep.get(tag)

        // ISO14443A 태그인 경우 연결 시도
        isoDep?.let {
            try {
                it.connect()
                it.timeout = 5000 // 5초 타임아웃
            } catch (e: Exception) {
                // 연결 실패는 무시하고 UID만 반환
            }
        }

        return NfcTagResult(
            uid = uid,
            techList = techList,
            isoDep = isoDep
        )
    }

    /**
     * ISO-DEP 명령 전송
     */
    fun sendIsoDepCommand(isoDep: IsoDep?, command: ByteArray): Flow<ByteArray> = callbackFlow {
        if (isoDep == null || !isoDep.isConnected) {
            close(Exception("IsoDep not connected"))
            return@callbackFlow
        }

        try {
            val response = isoDep.transceive(command)
            trySend(response)
        } catch (e: Exception) {
            close(e)
        } finally {
            awaitClose {
                isoDep.close()
            }
        }
    }

    private fun ByteArray.toHexString(): String =
        joinToString("") { "%02X".format(it) }
}
