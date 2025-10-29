package com.hypermob.mydrive3dx.data.hardware

import android.content.Context
import androidx.camera.core.*
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.core.content.ContextCompat
import androidx.lifecycle.LifecycleOwner
import com.google.mlkit.vision.common.InputImage
import com.google.mlkit.vision.face.FaceDetection
import com.google.mlkit.vision.face.FaceDetectorOptions
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.callbackFlow
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import javax.inject.Inject
import javax.inject.Singleton

/**
 * Face Detection Manager
 *
 * CameraX + ML Kit을 사용한 얼굴 인식 관리자
 */
@Singleton
class FaceDetectionManager @Inject constructor(
    @ApplicationContext private val context: Context
) {
    private var cameraExecutor: ExecutorService? = null
    private var imageAnalyzer: ImageAnalysis? = null
    private var cameraProvider: ProcessCameraProvider? = null

    /**
     * 얼굴 감지 결과 상태
     */
    sealed class FaceDetectionResult {
        object NoFace : FaceDetectionResult()
        data class FaceDetected(val faceCount: Int, val confidence: Float) : FaceDetectionResult()
        data class Error(val message: String) : FaceDetectionResult()
    }

    /**
     * 카메라 시작 및 얼굴 감지 Flow 반환
     */
    fun startFaceDetection(
        lifecycleOwner: LifecycleOwner,
        previewView: PreviewView
    ): Flow<FaceDetectionResult> = callbackFlow {
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

                // ML Kit Face Detector 설정
                val faceDetectorOptions = FaceDetectorOptions.Builder()
                    .setPerformanceMode(FaceDetectorOptions.PERFORMANCE_MODE_FAST)
                    .setContourMode(FaceDetectorOptions.CONTOUR_MODE_NONE)
                    .setClassificationMode(FaceDetectorOptions.CLASSIFICATION_MODE_NONE)
                    .setMinFaceSize(0.15f)
                    .build()

                val faceDetector = FaceDetection.getClient(faceDetectorOptions)

                // ImageAnalysis use case
                imageAnalyzer = ImageAnalysis.Builder()
                    .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                    .build()
                    .also { analysis ->
                        analysis.setAnalyzer(cameraExecutor!!) { imageProxy ->
                            processImageProxy(imageProxy, faceDetector) { result ->
                                trySend(result)
                            }
                        }
                    }

                // 전면 카메라 선택
                val cameraSelector = CameraSelector.DEFAULT_FRONT_CAMERA

                try {
                    // 기존 바인딩 해제
                    cameraProvider?.unbindAll()

                    // 카메라 바인딩
                    cameraProvider?.bindToLifecycle(
                        lifecycleOwner,
                        cameraSelector,
                        preview,
                        imageAnalyzer
                    )
                } catch (e: Exception) {
                    trySend(FaceDetectionResult.Error("Camera binding failed: ${e.message}"))
                }

            } catch (e: Exception) {
                trySend(FaceDetectionResult.Error("Camera initialization failed: ${e.message}"))
            }
        }, ContextCompat.getMainExecutor(context))

        awaitClose {
            stopFaceDetection()
        }
    }

    /**
     * 이미지 프록시를 처리하여 얼굴 감지
     */
    @androidx.camera.core.ExperimentalGetImage
    private fun processImageProxy(
        imageProxy: ImageProxy,
        faceDetector: com.google.mlkit.vision.face.FaceDetector,
        onResult: (FaceDetectionResult) -> Unit
    ) {
        val mediaImage = imageProxy.image
        if (mediaImage != null) {
            val image = InputImage.fromMediaImage(
                mediaImage,
                imageProxy.imageInfo.rotationDegrees
            )

            faceDetector.process(image)
                .addOnSuccessListener { faces ->
                    if (faces.isEmpty()) {
                        onResult(FaceDetectionResult.NoFace)
                    } else {
                        // 가장 큰 얼굴 선택
                        val largestFace = faces.maxByOrNull { it.boundingBox.width() * it.boundingBox.height() }
                        val faceSize = largestFace?.boundingBox?.let { box ->
                            (box.width() * box.height()).toFloat()
                        } ?: 0f
                        val imageSize = (image.width * image.height).toFloat()
                        val confidence = (faceSize / imageSize) * 100

                        onResult(FaceDetectionResult.FaceDetected(faces.size, confidence))
                    }
                }
                .addOnFailureListener { e ->
                    onResult(FaceDetectionResult.Error("Face detection failed: ${e.message}"))
                }
                .addOnCompleteListener {
                    imageProxy.close()
                }
        } else {
            imageProxy.close()
        }
    }

    /**
     * 얼굴 감지 중지
     */
    fun stopFaceDetection() {
        cameraProvider?.unbindAll()
        cameraExecutor?.shutdown()
        cameraExecutor = null
        imageAnalyzer = null
        cameraProvider = null
    }
}
