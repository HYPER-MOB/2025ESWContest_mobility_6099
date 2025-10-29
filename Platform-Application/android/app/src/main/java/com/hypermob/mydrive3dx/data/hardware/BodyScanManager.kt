package com.hypermob.mydrive3dx.data.hardware

import android.content.Context
import androidx.camera.core.*
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.core.content.ContextCompat
import androidx.lifecycle.LifecycleOwner
import com.google.mlkit.vision.common.InputImage
import com.google.mlkit.vision.pose.Pose
import com.google.mlkit.vision.pose.PoseDetection
import com.google.mlkit.vision.pose.PoseDetector
import com.google.mlkit.vision.pose.PoseLandmark
import com.google.mlkit.vision.pose.defaults.PoseDetectorOptions
import com.hypermob.mydrive3dx.domain.model.BodyScan
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.callbackFlow
import java.util.UUID
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import javax.inject.Inject
import javax.inject.Singleton
import kotlin.math.pow
import kotlin.math.sqrt

/**
 * Body Scan Manager
 *
 * Mediapipe Pose를 사용한 3D 바디스캔 및 체형 측정
 */
@Singleton
class BodyScanManager @Inject constructor(
    @ApplicationContext private val context: Context
) {
    private var cameraExecutor: ExecutorService? = null
    private var imageAnalyzer: ImageAnalysis? = null
    private var cameraProvider: ProcessCameraProvider? = null

    /**
     * 바디스캔 결과 상태
     */
    sealed class BodyScanResult {
        object Idle : BodyScanResult()
        object NoPersonDetected : BodyScanResult()
        data class PersonDetected(
            val pose: Pose,
            val confidence: Float
        ) : BodyScanResult()
        data class ScanComplete(val bodyScan: BodyScan) : BodyScanResult()
        data class Error(val message: String) : BodyScanResult()
    }

    /**
     * 바디스캔 시작
     */
    fun startBodyScan(
        lifecycleOwner: LifecycleOwner,
        previewView: PreviewView,
        userId: String
    ): Flow<BodyScanResult> = callbackFlow {
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

                // Pose Detector 설정
                val poseDetectorOptions = PoseDetectorOptions.Builder()
                    .setDetectorMode(PoseDetectorOptions.STREAM_MODE)
                    .build()

                val poseDetector = PoseDetection.getClient(poseDetectorOptions)

                // ImageAnalysis use case
                imageAnalyzer = ImageAnalysis.Builder()
                    .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                    .build()
                    .also { analysis ->
                        analysis.setAnalyzer(cameraExecutor!!) { imageProxy ->
                            processImageForBodyScan(imageProxy, poseDetector, userId) { result ->
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
                    trySend(BodyScanResult.Error("Camera binding failed: ${e.message}"))
                }

            } catch (e: Exception) {
                trySend(BodyScanResult.Error("Camera initialization failed: ${e.message}"))
            }
        }, ContextCompat.getMainExecutor(context))

        awaitClose {
            stopBodyScan()
        }
    }

    /**
     * 이미지 프록시를 처리하여 체형 측정
     */
    @androidx.camera.core.ExperimentalGetImage
    private fun processImageForBodyScan(
        imageProxy: ImageProxy,
        poseDetector: PoseDetector,
        userId: String,
        onResult: (BodyScanResult) -> Unit
    ) {
        val mediaImage = imageProxy.image
        if (mediaImage != null) {
            val image = InputImage.fromMediaImage(
                mediaImage,
                imageProxy.imageInfo.rotationDegrees
            )

            poseDetector.process(image)
                .addOnSuccessListener { pose ->
                    if (pose.allPoseLandmarks.isEmpty()) {
                        onResult(BodyScanResult.NoPersonDetected)
                    } else {
                        // 신뢰도 계산
                        val confidence = calculatePoseConfidence(pose)

                        if (confidence > 0.7f) {
                            // 충분히 신뢰도가 높으면 체형 측정
                            val bodyScan = calculateBodyMeasurements(pose, userId)
                            onResult(BodyScanResult.ScanComplete(bodyScan))
                        } else {
                            onResult(BodyScanResult.PersonDetected(pose, confidence))
                        }
                    }
                }
                .addOnFailureListener { e ->
                    onResult(BodyScanResult.Error("Pose detection failed: ${e.message}"))
                }
                .addOnCompleteListener {
                    imageProxy.close()
                }
        } else {
            imageProxy.close()
        }
    }

    /**
     * Pose 신뢰도 계산
     */
    private fun calculatePoseConfidence(pose: Pose): Float {
        val landmarks = pose.allPoseLandmarks
        if (landmarks.isEmpty()) return 0f

        val totalConfidence = landmarks.sumOf { it.inFrameLikelihood.toDouble() }
        return (totalConfidence / landmarks.size).toFloat()
    }

    /**
     * 체형 7지표 계산
     */
    private fun calculateBodyMeasurements(pose: Pose, userId: String): BodyScan {
        // 주요 랜드마크 추출
        val leftShoulder = pose.getPoseLandmark(PoseLandmark.LEFT_SHOULDER)
        val rightShoulder = pose.getPoseLandmark(PoseLandmark.RIGHT_SHOULDER)
        val leftHip = pose.getPoseLandmark(PoseLandmark.LEFT_HIP)
        val rightHip = pose.getPoseLandmark(PoseLandmark.RIGHT_HIP)
        val leftElbow = pose.getPoseLandmark(PoseLandmark.LEFT_ELBOW)
        val leftWrist = pose.getPoseLandmark(PoseLandmark.LEFT_WRIST)
        val leftKnee = pose.getPoseLandmark(PoseLandmark.LEFT_KNEE)
        val leftAnkle = pose.getPoseLandmark(PoseLandmark.LEFT_ANKLE)
        val nose = pose.getPoseLandmark(PoseLandmark.NOSE)

        // 픽셀 거리를 cm로 변환하는 간단한 추정 (실제로는 카메라 캘리브레이션 필요)
        val pixelToCm = 0.5 // 임시 변환 계수

        // 1. 어깨 너비
        val shoulderWidth = if (leftShoulder != null && rightShoulder != null) {
            calculateDistance(leftShoulder, rightShoulder) * pixelToCm
        } else 50.0

        // 2. 팔 길이 (어깨-팔꿈치-손목)
        val armLength = if (leftShoulder != null && leftElbow != null && leftWrist != null) {
            (calculateDistance(leftShoulder, leftElbow) + calculateDistance(leftElbow, leftWrist)) * pixelToCm
        } else 60.0

        // 3. 상체 길이 (어깨-엉덩이)
        val torsoLength = if (leftShoulder != null && leftHip != null) {
            calculateDistance(leftShoulder, leftHip) * pixelToCm
        } else 50.0

        // 4. 다리 길이 (엉덩이-무릎-발목)
        val legLength = if (leftHip != null && leftKnee != null && leftAnkle != null) {
            (calculateDistance(leftHip, leftKnee) + calculateDistance(leftKnee, leftAnkle)) * pixelToCm
        } else 90.0

        // 5. 머리 높이 (코-어깨 평균)
        val headHeight = if (nose != null && leftShoulder != null && rightShoulder != null) {
            val shoulderAvgY = (leftShoulder.position.y + rightShoulder.position.y) / 2
            kotlin.math.abs(nose.position.y - shoulderAvgY) * pixelToCm
        } else 25.0

        // 6. 눈 높이 (전체 키에서 추정)
        val eyeHeight = torsoLength + legLength - headHeight

        // 7. 앉은 키 (상체 + 머리)
        val sittingHeight = torsoLength + headHeight

        return BodyScan(
            scanId = UUID.randomUUID().toString(),
            userId = userId,
            timestamp = System.currentTimeMillis().toString(),
            sittingHeight = sittingHeight,
            shoulderWidth = shoulderWidth,
            armLength = armLength,
            headHeight = headHeight,
            eyeHeight = eyeHeight,
            legLength = legLength,
            torsoLength = torsoLength
        )
    }

    /**
     * 두 랜드마크 간 거리 계산 (픽셀)
     */
    private fun calculateDistance(landmark1: PoseLandmark, landmark2: PoseLandmark): Double {
        val dx = landmark1.position.x - landmark2.position.x
        val dy = landmark1.position.y - landmark2.position.y
        return sqrt(dx.toDouble().pow(2) + dy.toDouble().pow(2))
    }

    /**
     * 바디스캔 중지
     */
    fun stopBodyScan() {
        cameraProvider?.unbindAll()
        cameraExecutor?.shutdown()
        cameraExecutor = null
        imageAnalyzer = null
        cameraProvider = null
    }
}
