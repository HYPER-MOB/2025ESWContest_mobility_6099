package com.hypermob.mydrive3dx.domain.usecase

import android.graphics.Bitmap
import com.hypermob.mydrive3dx.domain.model.FaceRegisterResult
import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.repository.AuthRepository
import kotlinx.coroutines.flow.Flow
import javax.inject.Inject

/**
 * Register Face Use Case
 * 얼굴 등록 UseCase
 *
 * android_mvp의 FaceRegisterActivity 로직 기반
 */
class RegisterFaceUseCase @Inject constructor(
    private val authRepository: AuthRepository,
    private val compressImageUseCase: CompressImageUseCase
) {

    /**
     * 얼굴 이미지를 압축하여 서버에 등록
     *
     * @param bitmap 캡처된 얼굴 이미지
     * @return 등록 결과 (user_id, face_id, nfc_uid)
     */
    suspend operator fun invoke(bitmap: Bitmap): Flow<Result<FaceRegisterResult>> {
        // 1. 이미지 압축 (1MB 이하로)
        val compressedBytes = try {
            compressImageUseCase(bitmap)
        } catch (e: Exception) {
            return kotlinx.coroutines.flow.flow {
                emit(Result.Error(e))
            }
        }

        // 2. 서버에 업로드
        return authRepository.registerFace(compressedBytes)
    }
}
