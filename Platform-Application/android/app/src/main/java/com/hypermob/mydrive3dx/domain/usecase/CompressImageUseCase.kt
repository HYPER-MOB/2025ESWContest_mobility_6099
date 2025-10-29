package com.hypermob.mydrive3dx.domain.usecase

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import java.io.ByteArrayOutputStream
import javax.inject.Inject

/**
 * Compress Image Use Case
 * 이미지를 1MB 이하로 압축하는 UseCase
 *
 * android_mvp의 FaceRegisterActivity.kt 로직 기반
 */
class CompressImageUseCase @Inject constructor() {

    /**
     * 이미지를 1MB 이하로 압축
     *
     * @param bitmap 원본 비트맵
     * @param maxSize 최대 크기 (기본 1MB)
     * @return 압축된 바이트 배열
     * @throws IllegalArgumentException JPEG 헤더가 유효하지 않은 경우
     */
    operator fun invoke(bitmap: Bitmap, maxSize: Int = MAX_SIZE_BYTES): ByteArray {
        var compressedBitmap = bitmap

        // 1. 이미지 크기가 너무 크면 리사이즈
        if (bitmap.width > MAX_DIMENSION || bitmap.height > MAX_DIMENSION) {
            val scale = MAX_DIMENSION.toFloat() / maxOf(bitmap.width, bitmap.height)
            val newWidth = (bitmap.width * scale).toInt()
            val newHeight = (bitmap.height * scale).toInt()
            compressedBitmap = Bitmap.createScaledBitmap(bitmap, newWidth, newHeight, true)
        }

        // 2. 품질을 조정하면서 1MB 이하로 압축
        var quality = 90
        var compressedBytes: ByteArray

        do {
            val outputStream = ByteArrayOutputStream()
            compressedBitmap.compress(Bitmap.CompressFormat.JPEG, quality, outputStream)
            compressedBytes = outputStream.toByteArray()
            quality -= 10
        } while (compressedBytes.size > maxSize && quality > 0)

        // 3. JPEG 헤더 검증 (0xFF 0xD8)
        if (!isValidJpeg(compressedBytes)) {
            throw IllegalArgumentException("Invalid JPEG header after compression")
        }

        return compressedBytes
    }

    /**
     * 바이트 배열이 유효한 JPEG인지 검증
     * JPEG 파일은 항상 0xFF 0xD8로 시작
     */
    private fun isValidJpeg(bytes: ByteArray): Boolean {
        if (bytes.size < 2) return false
        return bytes[0] == 0xFF.toByte() && bytes[1] == 0xD8.toByte()
    }

    /**
     * 바이트 배열을 비트맵으로 디코딩
     */
    fun decodeBytes(bytes: ByteArray): Bitmap? {
        return BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
    }

    companion object {
        private const val MAX_SIZE_BYTES = 1 * 1024 * 1024  // 1MB
        private const val MAX_DIMENSION = 1920  // 1920x1920 최대 크기
    }
}
