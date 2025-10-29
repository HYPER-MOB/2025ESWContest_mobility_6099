package com.hypermob.mydrive3dx.presentation.bodyscan

import android.graphics.Bitmap
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.hypermob.mydrive3dx.domain.model.BodyScan
import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.repository.BodyScanRepository
import com.hypermob.mydrive3dx.domain.usecase.CompressImageUseCase
import com.hypermob.mydrive3dx.domain.usecase.CreateBodyProfileUseCase
import com.hypermob.mydrive3dx.domain.usecase.GetCurrentUserUseCase
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import javax.inject.Inject

/**
 * Body Scan ViewModel
 *
 * 3D 바디스캔 및 프로필 생성 로직
 */
@HiltViewModel
class BodyScanViewModel @Inject constructor(
    private val getCurrentUserUseCase: GetCurrentUserUseCase,
    private val createBodyProfileUseCase: CreateBodyProfileUseCase,
    private val bodyScanRepository: BodyScanRepository,
    private val compressImageUseCase: CompressImageUseCase,
    private val tokenManager: com.hypermob.mydrive3dx.data.local.TokenManager
) : ViewModel() {

    private val _uiState = MutableStateFlow(BodyScanUiState())
    val uiState: StateFlow<BodyScanUiState> = _uiState.asStateFlow()

    private var userId: String? = null
    private var bodyImageS3Path: String? = null
    private var measurementData: com.hypermob.mydrive3dx.domain.repository.MeasurementResult? = null

    init {
        loadCurrentUser()
    }

    /**
     * 현재 사용자 정보 로드
     */
    private fun loadCurrentUser() {
        viewModelScope.launch {
            getCurrentUserUseCase().collect { result ->
                when (result) {
                    is Result.Success -> {
                        userId = result.data.userId
                    }
                    is Result.Error -> {
                        // 에러 시 TokenManager에서 userId 가져오기
                        val savedUserId = tokenManager.getUserId()
                        userId = savedUserId ?: "test_user_${System.currentTimeMillis()}"
                        // 에러 메시지 표시 안함
                    }
                    is Result.Loading -> {}
                }
            }
        }
    }

    /**
     * 스캔 시작
     */
    fun startScan() {
        _uiState.update {
            it.copy(
                isScanning = true,
                scanStep = ScanStep.SCANNING,
                errorMessage = null
            )
        }
    }

    /**
     * 사람 감지됨
     */
    fun onPersonDetected(confidence: Float) {
        _uiState.update {
            it.copy(
                isPersonDetected = true,
                scanConfidence = confidence
            )
        }
    }

    /**
     * 사람 감지 안됨
     */
    fun onNoPersonDetected() {
        _uiState.update {
            it.copy(
                isPersonDetected = false,
                scanConfidence = 0f
            )
        }
    }

    /**
     * 스캔 완료
     */
    fun onScanComplete(bodyScan: BodyScan) {
        _uiState.update {
            it.copy(
                isScanning = false,
                bodyScanData = bodyScan,
                scanStep = ScanStep.PREVIEW
            )
        }
    }

    /**
     * 스캔 에러
     */
    fun onScanError(message: String) {
        _uiState.update {
            it.copy(
                isScanning = false,
                errorMessage = message,
                scanStep = ScanStep.INSTRUCTION
            )
        }
    }

    /**
     * 프로필 생성
     */
    fun createProfile(vehicleId: String? = null) {
        val bodyScanData = _uiState.value.bodyScanData
        // userId가 없으면 임시 ID 사용
        val currentUserId = userId ?: "test_user_${System.currentTimeMillis()}"

        if (bodyScanData == null) {
            _uiState.update {
                it.copy(errorMessage = "스캔 데이터가 없습니다")
            }
            return
        }

        _uiState.update {
            it.copy(
                isCreatingProfile = true,
                scanStep = ScanStep.CREATING,
                errorMessage = null
            )
        }

        viewModelScope.launch {
            createBodyProfileUseCase(currentUserId, bodyScanData, vehicleId).collect { result ->
                when (result) {
                    is Result.Loading -> {
                        _uiState.update { it.copy(isCreatingProfile = true) }
                    }
                    is Result.Success -> {
                        _uiState.update {
                            it.copy(
                                isCreatingProfile = false,
                                bodyProfile = result.data,
                                isProfileCreated = true,
                                scanStep = ScanStep.COMPLETE
                            )
                        }
                    }
                    is Result.Error -> {
                        _uiState.update {
                            it.copy(
                                isCreatingProfile = false,
                                errorMessage = result.exception.message ?: "프로필 생성 실패",
                                scanStep = ScanStep.PREVIEW
                            )
                        }
                    }
                }
            }
        }
    }

    /**
     * 재스캔
     */
    fun rescan() {
        _uiState.update {
            BodyScanUiState(scanStep = ScanStep.SCANNING, isScanning = true)
        }
    }

    /**
     * 초기화
     */
    fun reset() {
        _uiState.value = BodyScanUiState()
    }

    /**
     * 전신 사진 촬영 완료 (시나리오1 - Step 7)
     */
    fun onBodyPhotoCaptured(bitmap: Bitmap) {
        viewModelScope.launch {
            try {
                // userId가 없으면 임시 ID 사용
                val currentUserId = userId ?: "test_user_${System.currentTimeMillis()}"

                _uiState.update {
                    it.copy(
                        scanStep = ScanStep.BODY_PHOTO_CAPTURED,
                        capturedBodyImage = bitmap
                    )
                }

                // 이미지 압축
                val compressedBytes = compressImageUseCase(bitmap)

                // 서버에 업로드
                bodyScanRepository.uploadBodyImage(currentUserId, compressedBytes, null).collect { result ->
                    when (result) {
                        is Result.Loading -> {
                            _uiState.update { it.copy(isUploadingImage = true) }
                        }
                        is Result.Success -> {
                            bodyImageS3Path = result.data.s3Path
                            _uiState.update {
                                it.copy(
                                    isUploadingImage = false,
                                    isBodyImageUploaded = true,
                                    scanStep = ScanStep.HEIGHT_INPUT,
                                    uploadedBodyDataId = result.data.bodyDataId
                                )
                            }
                        }
                        is Result.Error -> {
                            _uiState.update {
                                it.copy(
                                    isUploadingImage = false,
                                    errorMessage = result.exception.message ?: "이미지 업로드 실패",
                                    scanStep = ScanStep.PREVIEW
                                )
                            }
                        }
                    }
                }
            } catch (e: Exception) {
                _uiState.update {
                    it.copy(
                        isUploadingImage = false,
                        errorMessage = e.message ?: "이미지 압축 실패",
                        scanStep = ScanStep.PREVIEW
                    )
                }
            }
        }
    }

    /**
     * 사람 키 입력 완료 (시나리오1 - Step 8, 9)
     */
    fun onHeightConfirmed(heightCm: Int) {
        viewModelScope.launch {
            val currentUserId = userId ?: "test_user_${System.currentTimeMillis()}"
            val s3Path = bodyImageS3Path

            if (s3Path == null) {
                _uiState.update {
                    it.copy(errorMessage = "이미지 업로드가 완료되지 않았습니다")
                }
                return@launch
            }

            _uiState.update {
                it.copy(
                    scanStep = ScanStep.GENERATING_BODY_DATA,
                    userHeight = heightCm.toFloat()
                )
            }

            // Step 9: 체형 데이터 생성
            bodyScanRepository.generateMeasurement(currentUserId, s3Path, heightCm).collect { result ->
                when (result) {
                    is Result.Loading -> {
                        // 로딩 중
                    }
                    is Result.Success -> {
                        measurementData = result.data
                        // Step 10: 맞춤형 차량 프로필 생성
                        generateRecommendation(heightCm)
                    }
                    is Result.Error -> {
                        _uiState.update {
                            it.copy(
                                errorMessage = result.exception.message,
                                scanStep = ScanStep.HEIGHT_INPUT
                            )
                        }
                    }
                }
            }
        }
    }

    /**
     * 맞춤형 차량 프로필 생성 (시나리오1 - Step 10)
     */
    private suspend fun generateRecommendation(heightCm: Int) {
        val currentUserId = userId ?: "test_user_${System.currentTimeMillis()}"
        val measurements = measurementData

        if (measurements == null) {
            _uiState.update {
                it.copy(errorMessage = "체형 데이터 생성이 완료되지 않았습니다")
            }
            return
        }

        _uiState.update {
            it.copy(scanStep = ScanStep.GENERATING_PROFILE)
        }

        bodyScanRepository.generateRecommendationWithMeasurements(
            userId = currentUserId,
            height = heightCm.toDouble(),
            upperArm = measurements.upperArmCm ?: 0.0,
            forearm = measurements.forearmCm ?: 0.0,
            thigh = measurements.thighCm ?: 0.0,
            calf = measurements.calfCm ?: 0.0,
            torso = measurements.torsoCm ?: 0.0
        ).collect { result ->
            when (result) {
                is Result.Loading -> {
                    // 로딩 중
                }
                is Result.Success -> {
                    // 추천 성공 후 프로필 저장
                    val recommendation = result.data
                    saveProfileToDatabase(currentUserId, measurements, heightCm, recommendation)
                }
                is Result.Error -> {
                    _uiState.update {
                        it.copy(
                            errorMessage = result.exception.message,
                            scanStep = ScanStep.HEIGHT_INPUT
                        )
                    }
                }
            }
        }
    }

    /**
     * 사람 키 입력 업데이트
     */
    fun updateHeight(height: Float) {
        _uiState.update {
            it.copy(userHeight = height)
        }
    }

    /**
     * 추천 데이터를 프로필로 저장
     */
    private fun saveProfileToDatabase(
        userId: String,
        measurements: com.hypermob.mydrive3dx.domain.repository.MeasurementResult,
        heightCm: Int,
        recommendation: com.hypermob.mydrive3dx.domain.repository.RecommendationResult
    ) {
        viewModelScope.launch {
            try {
                // BodyScan 데이터 생성
                val bodyScanData = BodyScan(
                    scanId = "scan_${System.currentTimeMillis()}",
                    userId = userId,
                    timestamp = java.time.LocalDateTime.now().toString(),
                    sittingHeight = heightCm.toDouble(),
                    shoulderWidth = measurements.shoulderWidth ?: 0.0,
                    armLength = measurements.armLength ?: 0.0,
                    headHeight = 0.0, // Not available
                    eyeHeight = 0.0, // Not available
                    legLength = measurements.legLength ?: 0.0,
                    torsoLength = measurements.torsoHeight ?: 0.0,
                    weight = 0.0, // Not available
                    height = heightCm.toDouble()
                )

                // Profile is automatically saved via /profiles/{user_id} after recommend
                // Mark as complete
                _uiState.update {
                    it.copy(
                        scanStep = ScanStep.COMPLETE,
                        isProfileCreated = true,
                        profileId = "profile_${System.currentTimeMillis()}"
                    )
                }
            } catch (e: Exception) {
                _uiState.update {
                    it.copy(
                        errorMessage = e.message,
                        scanStep = ScanStep.HEIGHT_INPUT
                    )
                }
            }
        }
    }

}
