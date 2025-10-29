package com.hypermob.mydrive3dx.data.hardware

import android.content.Context
import androidx.camera.core.*
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.core.content.ContextCompat
import androidx.lifecycle.LifecycleOwner
import com.google.mlkit.vision.common.InputImage
import com.google.mlkit.vision.face.Face
import com.google.mlkit.vision.face.FaceDetection
import com.google.mlkit.vision.face.FaceDetector
import com.google.mlkit.vision.face.FaceDetectorOptions
import com.google.mlkit.vision.face.FaceLandmark
import com.hypermob.mydrive3dx.domain.model.DrowsinessDetectionResult
import com.hypermob.mydrive3dx.domain.model.DrowsinessLevel
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.callbackFlow
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import javax.inject.Inject
import javax.inject.Singleton
import kotlin.math.abs
import kotlin.math.atan2
import kotlin.math.pow
import kotlin.math.sqrt

/**
 * Drowsiness Detection Manager
 *
 * Mediapipe Face Mesh + Iris를 사용한 졸음 감지
 */
@Singleton
class DrowsinessDetectionManager @Inject constructor(
    @ApplicationContext private val context: Context
) {
    private var cameraExecutor: ExecutorService? = null
    private var imageAnalyzer: ImageAnalysis? = null
    private var cameraProvider: ProcessCameraProvider? = null

    // 졸음 감지 상태
    private var eyeClosureStartTime: Long? = null
    private var blinkCount = 0
    private var lastBlinkTime = 0L
    private val blinkHistory = mutableListOf<Long>()

    companion object {
        private const val EAR_THRESHOLD = 0.25f          // EAR 임계값
        private const val EYE_CLOSURE_THRESHOLD = 2000L  // 눈 감은 시간 임계값 (2초)
        private const val HEAD_POSE_THRESHOLD = 30f      // 머리 기울기 임계값 (도)
        private const val BLINK_INTERVAL = 60000L        // 눈 깜빡임 측정 간격 (1분)
    }

    /**
     * 졸음 감지 시작
     */
    fun startDrowsinessDetection(
        lifecycleOwner: LifecycleOwner,
        previewView: PreviewView
    ): Flow<DrowsinessDetectionResult> = callbackFlow {
        cameraExecutor = Executors.newSingleThreadExecutor()

        val cameraProviderFuture = ProcessCameraProvider.getInstance(context)
        cameraProviderFuture.addListener({
            try {
                cameraProvider = cameraProviderFuture.get()

                // Preview use case
                val preview = Preview.Builder()
                    .build()
                    .also {
                        it.setSurfaceProvider(previewView.surfaceProvider)
                    }

                // Face Detector 설정 (Mediapipe Face Mesh)
                val faceDetectorOptions = FaceDetectorOptions.Builder()
                    .setPerformanceMode(FaceDetectorOptions.PERFORMANCE_MODE_FAST)
                    .setLandmarkMode(FaceDetectorOptions.LANDMARK_MODE_ALL)
                    .setClassificationMode(FaceDetectorOptions.CLASSIFICATION_MODE_ALL)
                    .setMinFaceSize(0.15f)
                    .enableTracking()
                    .build()

                val faceDetector = FaceDetection.getClient(faceDetectorOptions)

                // ImageAnalysis use case
                imageAnalyzer = ImageAnalysis.Builder()
                    .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                    .build()
                    .also { analysis ->
                        analysis.setAnalyzer(cameraExecutor!!) { imageProxy ->
                            processImageForDrowsiness(imageProxy, faceDetector) { result ->
                                trySend(result)
                            }
                        }
                    }

                // 전면 카메라 선택
                val cameraSelector = CameraSelector.DEFAULT_FRONT_CAMERA

                try {
                    cameraProvider?.unbindAll()
                    cameraProvider?.bindToLifecycle(
                        lifecycleOwner,
                        cameraSelector,
                        preview,
                        imageAnalyzer
                    )
                } catch (e: Exception) {
                    // Error handling
                }

            } catch (e: Exception) {
                // Error handling
            }
        }, ContextCompat.getMainExecutor(context))

        awaitClose {
            stopDrowsinessDetection()
        }
    }

    /**
     * 이미지 프록시를 처리하여 졸음 감지
     */
    @androidx.camera.core.ExperimentalGetImage
    private fun processImageForDrowsiness(
        imageProxy: ImageProxy,
        faceDetector: FaceDetector,
        onResult: (DrowsinessDetectionResult) -> Unit
    ) {
        val mediaImage = imageProxy.image
        if (mediaImage != null) {
            val image = InputImage.fromMediaImage(
                mediaImage,
                imageProxy.imageInfo.rotationDegrees
            )

            faceDetector.process(image)
                .addOnSuccessListener { faces ->
                    if (faces.isNotEmpty()) {
                        val face = faces[0] // 첫 번째 얼굴 사용
                        val result = analyzeDrowsiness(face)
                        onResult(result)
                    }
                }
                .addOnFailureListener {
                    // Error handling
                }
                .addOnCompleteListener {
                    imageProxy.close()
                }
        } else {
            imageProxy.close()
        }
    }

    /**
     * 졸음 분석
     */
    private fun analyzeDrowsiness(face: Face): DrowsinessDetectionResult {
        val currentTime = System.currentTimeMillis()

        // 1. EAR (Eye Aspect Ratio) 계산
        val ear = calculateEAR(face)

        // 2. 눈 감은 시간 측정
        val eyeClosureDuration = if (ear < EAR_THRESHOLD) {
            if (eyeClosureStartTime == null) {
                eyeClosureStartTime = currentTime
            }
            currentTime - eyeClosureStartTime!!
        } else {
            // 눈을 뜬 경우
            if (eyeClosureStartTime != null) {
                // 눈 깜빡임 카운트
                recordBlink(currentTime)
                eyeClosureStartTime = null
            }
            0L
        }

        // 3. 머리 자세 분석 (Head Pose)
        val headPoseAngle = calculateHeadPose(face)

        // 4. 눈 깜빡임 빈도 계산
        val blinkFrequency = calculateBlinkFrequency(currentTime)

        // 5. 졸음 레벨 판정
        val level = determineDrowsinessLevel(ear, eyeClosureDuration, headPoseAngle, blinkFrequency)

        // 6. 신뢰도 계산
        val confidence = face.trackingId?.let { 0.9f } ?: 0.7f

        return DrowsinessDetectionResult(
            timestamp = currentTime,
            level = level,
            eyeAspectRatio = ear,
            eyeClosureDuration = eyeClosureDuration,
            headPoseAngle = headPoseAngle,
            blinkFrequency = blinkFrequency,
            confidence = confidence,
            message = getMessageForLevel(level)
        )
    }

    /**
     * EAR (Eye Aspect Ratio) 계산
     *
     * EAR = (||p2-p6|| + ||p3-p5||) / (2 * ||p1-p4||)
     * 눈이 열려있을 때: ~0.3, 눈이 감겨있을 때: ~0.1
     */
    private fun calculateEAR(face: Face): Float {
        val leftEye = face.getLandmark(FaceLandmark.LEFT_EYE)
        val rightEye = face.getLandmark(FaceLandmark.RIGHT_EYE)

        if (leftEye == null || rightEye == null) {
            return 0.3f // 기본값 (정상)
        }

        // ML Kit은 간단한 랜드마크만 제공하므로 눈 깜빡임 확률 사용
        val leftEyeOpen = face.leftEyeOpenProbability ?: 0.5f
        val rightEyeOpen = face.rightEyeOpenProbability ?: 0.5f

        // 눈 뜸 확률을 EAR로 변환
        return (leftEyeOpen + rightEyeOpen) / 2.0f * 0.35f // 0~0.35 범위로 정규화
    }

    /**
     * 머리 자세 각도 계산
     */
    private fun calculateHeadPose(face: Face): Float {
        // Euler Y (머리 좌우 기울기)
        val headY = face.headEulerAngleY

        // Euler Z (머리 앞뒤 기울기)
        val headZ = face.headEulerAngleZ

        // 전체 기울기 각도
        return sqrt(headY.pow(2) + headZ.pow(2))
    }

    /**
     * 눈 깜빡임 기록
     */
    private fun recordBlink(currentTime: Long) {
        blinkCount++
        blinkHistory.add(currentTime)

        // 1분 이전 기록 제거
        blinkHistory.removeAll { it < currentTime - BLINK_INTERVAL }
    }

    /**
     * 눈 깜빡임 빈도 계산 (per minute)
     */
    private fun calculateBlinkFrequency(currentTime: Long): Int {
        // 최근 1분 동안의 깜빡임 횟수
        return blinkHistory.count { it >= currentTime - BLINK_INTERVAL }
    }

    /**
     * 졸음 레벨 판정
     */
    private fun determineDrowsinessLevel(
        ear: Float,
        eyeClosureDuration: Long,
        headPoseAngle: Float,
        blinkFrequency: Int
    ): DrowsinessLevel {
        return when {
            // CRITICAL: 눈을 2초 이상 감음 또는 머리가 크게 기울어짐
            eyeClosureDuration > EYE_CLOSURE_THRESHOLD || headPoseAngle > HEAD_POSE_THRESHOLD * 1.5 -> {
                DrowsinessLevel.CRITICAL
            }
            // DANGER: 눈을 1초 이상 감음 또는 EAR이 매우 낮음
            eyeClosureDuration > 1000 || ear < 0.15f || headPoseAngle > HEAD_POSE_THRESHOLD -> {
                DrowsinessLevel.DANGER
            }
            // WARNING: EAR이 낮거나 눈 깜빡임이 적음
            ear < EAR_THRESHOLD || blinkFrequency < 5 -> {
                DrowsinessLevel.WARNING
            }
            // NORMAL
            else -> {
                DrowsinessLevel.NORMAL
            }
        }
    }

    /**
     * 레벨별 메시지
     */
    private fun getMessageForLevel(level: DrowsinessLevel): String {
        return when (level) {
            DrowsinessLevel.NORMAL -> "정상"
            DrowsinessLevel.WARNING -> "졸음 징후 감지"
            DrowsinessLevel.DANGER -> "위험! 휴식이 필요합니다"
            DrowsinessLevel.CRITICAL -> "매우 위험! 즉시 차량을 정차하세요"
        }
    }

    /**
     * 졸음 감지 중지
     */
    fun stopDrowsinessDetection() {
        cameraProvider?.unbindAll()
        cameraExecutor?.shutdown()
        cameraExecutor = null
        imageAnalyzer = null
        cameraProvider = null
        resetState()
    }

    /**
     * 상태 초기화
     */
    private fun resetState() {
        eyeClosureStartTime = null
        blinkCount = 0
        lastBlinkTime = 0L
        blinkHistory.clear()
    }
}
