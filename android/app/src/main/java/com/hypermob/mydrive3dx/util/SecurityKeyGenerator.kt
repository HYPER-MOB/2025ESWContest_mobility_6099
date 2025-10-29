package com.hypermob.mydrive3dx.util

import java.security.SecureRandom
import java.util.UUID

/**
 * Security Key Generator
 *
 * BLE Advertising 인증용 키 생성 유틸리티
 */
object SecurityKeyGenerator {

    /**
     * UUID 기반 인증 키 생성
     *
     * @return UUID 문자열 (예: "550e8400-e29b-41d4-a716-446655440000")
     */
    fun generateUuidKey(): String {
        return UUID.randomUUID().toString()
    }

    /**
     * 16진수 보안 키 생성
     *
     * @param length 키 길이 (바이트 단위, 기본 16 = 128bit)
     * @return 16진수 문자열 (예: "A1B2C3D4E5F6...")
     */
    fun generateHexKey(length: Int = 16): String {
        val random = SecureRandom()
        val bytes = ByteArray(length)
        random.nextBytes(bytes)

        return bytes.joinToString("") { "%02X".format(it) }
    }

    /**
     * 사용자 친화적인 인증 코드 생성
     *
     * @param length 코드 길이 (기본 8자리)
     * @return 숫자와 대문자로 구성된 코드 (예: "A3B7C2D9")
     */
    fun generateFriendlyCode(length: Int = 8): String {
        val chars = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789" // 헷갈리는 문자 제외 (I, O, 0, 1)
        val random = SecureRandom()

        return (1..length)
            .map { chars[random.nextInt(chars.length)] }
            .joinToString("")
    }

    /**
     * BLE Advertising에 사용할 인증 키 생성
     *
     * UUID + Timestamp 조합으로 유일성 보장
     */
    fun generateBleAuthKey(): BleAuthKey {
        val uuid = generateUuidKey()
        val shortKey = generateFriendlyCode(8)
        val timestamp = System.currentTimeMillis()

        return BleAuthKey(
            uuid = uuid,
            shortKey = shortKey,
            timestamp = timestamp,
            fullKey = "$uuid-$timestamp"
        )
    }

    /**
     * 기존 키의 유효성 검증
     *
     * @param key 검증할 키
     * @param maxAgeMs 최대 유효 기간 (밀리초, 기본 24시간)
     * @return 유효 여부
     */
    fun validateKey(key: BleAuthKey, maxAgeMs: Long = 24 * 60 * 60 * 1000): Boolean {
        val currentTime = System.currentTimeMillis()
        val keyAge = currentTime - key.timestamp

        return keyAge <= maxAgeMs
    }
}

/**
 * BLE 인증 키 데이터 클래스
 */
data class BleAuthKey(
    val uuid: String,          // UUID 전체
    val shortKey: String,      // 사용자 친화적 짧은 코드
    val timestamp: Long,       // 생성 시간
    val fullKey: String        // UUID + Timestamp 조합
) {
    /**
     * 표시용 짧은 키 (앞 8자리)
     */
    fun getDisplayKey(): String {
        return uuid.take(8).uppercase()
    }

    /**
     * QR 코드용 키
     */
    fun getQrCode(): String {
        return fullKey
    }
}
