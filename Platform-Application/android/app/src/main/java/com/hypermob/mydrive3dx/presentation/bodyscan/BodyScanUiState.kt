package com.hypermob.mydrive3dx.presentation.bodyscan

import com.hypermob.mydrive3dx.domain.model.BodyProfile
import com.hypermob.mydrive3dx.domain.model.BodyScan

/**
 * Body Scan UI State
 */
data class BodyScanUiState(
    val isScanning: Boolean = false,
    val isPersonDetected: Boolean = false,
    val scanConfidence: Float = 0f,
    val bodyScanData: BodyScan? = null,
    val bodyProfile: BodyProfile? = null,
    val isCreatingProfile: Boolean = false,
    val isProfileCreated: Boolean = false,
    val errorMessage: String? = null,
    val scanStep: ScanStep = ScanStep.INSTRUCTION,
    // 새로 추가된 필드들 (시나리오1)
    val isUploadingImage: Boolean = false,
    val isBodyImageUploaded: Boolean = false,
    val uploadedBodyDataId: String? = null,
    val userHeight: Float? = null,
    val showHeightDialog: Boolean = false,
    val capturedBodyImage: android.graphics.Bitmap? = null,
    val profileId: String? = null
)

/**
 * Scan Step
 */
enum class ScanStep {
    INSTRUCTION,        // 안내 화면
    SCANNING,          // 스캔 중
    PREVIEW,           // 스캔 결과 미리보기
    BODY_PHOTO_CAPTURED, // 전신 사진 촬영 완료 (시나리오1)
    HEIGHT_INPUT,      // 키 입력 (시나리오1)
    UPLOADING,         // 이미지 업로드 중 (시나리오1)
    GENERATING_BODY_DATA, // 체형 데이터 생성 중 (시나리오1 - Step 9)
    GENERATING_PROFILE, // 맞춤형 차량 프로필 생성 중 (시나리오1 - Step 10)
    CREATING,          // 프로필 생성 중
    COMPLETE           // 완료
}
