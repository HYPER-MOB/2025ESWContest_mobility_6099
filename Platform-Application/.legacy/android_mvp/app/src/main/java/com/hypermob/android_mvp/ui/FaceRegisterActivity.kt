package com.hypermob.android_mvp.ui

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Matrix
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.camera.core.*
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.core.content.ContextCompat
import androidx.lifecycle.lifecycleScope
import com.hypermob.android_mvp.data.api.RetrofitClient
import com.hypermob.android_mvp.databinding.ActivityFaceRegisterBinding
import com.hypermob.android_mvp.util.PreferenceManager
import kotlinx.coroutines.launch
import okhttp3.MediaType.Companion.toMediaTypeOrNull
import okhttp3.MultipartBody
import okhttp3.RequestBody.Companion.toRequestBody
import java.io.ByteArrayOutputStream
import java.io.File
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

/**
 * Face Registration Activity
 * - Captures face image using CameraX
 * - Uploads to server via POST /auth/face
 * - Saves user_id and face_id to SharedPreferences
 */
class FaceRegisterActivity : AppCompatActivity() {

    private lateinit var binding: ActivityFaceRegisterBinding
    private lateinit var prefManager: PreferenceManager
    private lateinit var cameraExecutor: ExecutorService

    private var imageCapture: ImageCapture? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityFaceRegisterBinding.inflate(layoutInflater)
        setContentView(binding.root)

        prefManager = PreferenceManager(this)
        cameraExecutor = Executors.newSingleThreadExecutor()

        setupCamera()
        setupUI()
    }

    private fun setupCamera() {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(this)

        cameraProviderFuture.addListener({
            val cameraProvider = cameraProviderFuture.get()

            // Preview
            val preview = Preview.Builder()
                .build()
                .also {
                    it.setSurfaceProvider(binding.previewView.surfaceProvider)
                }

            // Image capture with explicit JPEG format
            imageCapture = ImageCapture.Builder()
                .setCaptureMode(ImageCapture.CAPTURE_MODE_MINIMIZE_LATENCY)
                .setTargetRotation(binding.previewView.display.rotation)
                .setJpegQuality(95) // High quality JPEG
                .build()

            // Select front camera
            val cameraSelector = CameraSelector.DEFAULT_FRONT_CAMERA

            try {
                cameraProvider.unbindAll()
                cameraProvider.bindToLifecycle(
                    this, cameraSelector, preview, imageCapture
                )
            } catch (e: Exception) {
                Log.e(TAG, "Camera binding failed", e)
                Toast.makeText(this, "Camera initialization failed", Toast.LENGTH_SHORT).show()
            }

        }, ContextCompat.getMainExecutor(this))
    }

    private fun setupUI() {
        binding.btnCapture.setOnClickListener {
            captureImage()
        }
    }

    private fun captureImage() {
        val imageCapture = imageCapture ?: return

        // Capture to temporary file to get proper JPEG output
        val photoFile = File(cacheDir, "temp_face_${System.currentTimeMillis()}.jpg")
        val outputOptions = ImageCapture.OutputFileOptions.Builder(photoFile).build()

        imageCapture.takePicture(
            outputOptions,
            ContextCompat.getMainExecutor(this),
            object : ImageCapture.OnImageSavedCallback {
                override fun onImageSaved(output: ImageCapture.OutputFileResults) {
                    try {
                        // Read the JPEG file
                        val jpegBytes = photoFile.readBytes()

                        // Verify JPEG header
                        val byte0 = jpegBytes[0].toInt() and 0xFF
                        val byte1 = jpegBytes[1].toInt() and 0xFF
                        Log.d(TAG, "Captured JPEG file: ${jpegBytes.size} bytes, header: 0x${byte0.toString(16)} 0x${byte1.toString(16)}")

                        if (byte0 == 0xFF && byte1 == 0xD8) {
                            // Convert to Bitmap for processing
                            val bitmap = BitmapFactory.decodeByteArray(jpegBytes, 0, jpegBytes.size)
                            if (bitmap != null) {
                                Log.d(TAG, "Bitmap decoded: ${bitmap.width}x${bitmap.height}")
                                uploadFaceImage(bitmap)
                            } else {
                                Log.e(TAG, "Failed to decode JPEG to Bitmap")
                                Toast.makeText(
                                    this@FaceRegisterActivity,
                                    "Failed to decode image",
                                    Toast.LENGTH_SHORT
                                ).show()
                            }
                        } else {
                            Log.e(TAG, "Captured file is not JPEG! Got: 0x${byte0.toString(16)} 0x${byte1.toString(16)}")
                            Toast.makeText(
                                this@FaceRegisterActivity,
                                "Camera returned invalid format",
                                Toast.LENGTH_SHORT
                            ).show()
                        }
                    } catch (e: Exception) {
                        Log.e(TAG, "Failed to read captured image", e)
                        Toast.makeText(
                            this@FaceRegisterActivity,
                            "Failed to read image: ${e.message}",
                            Toast.LENGTH_SHORT
                        ).show()
                    } finally {
                        // Clean up temp file
                        photoFile.delete()
                    }
                }

                override fun onError(exception: ImageCaptureException) {
                    Log.e(TAG, "Image capture failed", exception)
                    Toast.makeText(
                        this@FaceRegisterActivity,
                        "Capture failed: ${exception.message}",
                        Toast.LENGTH_SHORT
                    ).show()
                    photoFile.delete()
                }
            }
        )
    }

    private fun rotateBitmap(bitmap: Bitmap, degrees: Float): Bitmap {
        val matrix = Matrix().apply {
            postRotate(degrees)
            postScale(-1f, 1f) // Mirror for front camera
        }
        return Bitmap.createBitmap(bitmap, 0, 0, bitmap.width, bitmap.height, matrix, true)
    }

    /**
     * Compress image to ensure it's under 1MB and validate JPEG format
     * - First resize if too large
     * - Then adjust quality
     * - Verify JPEG header (0xFF 0xD8)
     */
    private fun compressImageUnder1MB(bitmap: Bitmap): ByteArray {
        val maxSizeBytes = 1024 * 1024 // 1MB
        var currentBitmap = bitmap

        // Step 1: Resize if image is too large (max 1920x1920)
        val maxDimension = 1920
        if (bitmap.width > maxDimension || bitmap.height > maxDimension) {
            val scale = minOf(
                maxDimension.toFloat() / bitmap.width,
                maxDimension.toFloat() / bitmap.height
            )
            val newWidth = (bitmap.width * scale).toInt()
            val newHeight = (bitmap.height * scale).toInt()
            currentBitmap = Bitmap.createScaledBitmap(bitmap, newWidth, newHeight, true)
            Log.d(TAG, "Resized to ${newWidth}x${newHeight}")
        }

        // Step 2: Compress with decreasing quality until under 1MB
        var quality = 90
        var compressedBytes: ByteArray

        do {
            val outputStream = ByteArrayOutputStream()
            // Force JPEG format compression
            val success = currentBitmap.compress(Bitmap.CompressFormat.JPEG, quality, outputStream)

            if (!success) {
                Log.e(TAG, "Failed to compress bitmap at quality $quality")
                throw IllegalStateException("Bitmap compression failed")
            }

            compressedBytes = outputStream.toByteArray()

            val sizeKB = compressedBytes.size / 1024
            Log.d(TAG, "Compressed at quality $quality: $sizeKB KB")

            if (compressedBytes.size <= maxSizeBytes) {
                break
            }

            quality -= 10
        } while (quality > 10)

        // If still too large after quality=10, resize further
        if (compressedBytes.size > maxSizeBytes) {
            val scale = 0.8f
            val newWidth = (currentBitmap.width * scale).toInt()
            val newHeight = (currentBitmap.height * scale).toInt()
            currentBitmap = Bitmap.createScaledBitmap(currentBitmap, newWidth, newHeight, true)

            val outputStream = ByteArrayOutputStream()
            currentBitmap.compress(Bitmap.CompressFormat.JPEG, 80, outputStream)
            compressedBytes = outputStream.toByteArray()

            Log.d(TAG, "Further resized to ${newWidth}x${newHeight}: ${compressedBytes.size / 1024} KB")
        }

        // Step 3: Verify JPEG header (must start with 0xFF 0xD8)
        if (compressedBytes.size >= 2) {
            val isJpeg = (compressedBytes[0].toInt() and 0xFF) == 0xFF &&
                         (compressedBytes[1].toInt() and 0xFF) == 0xD8

            Log.d(TAG, "JPEG header check: $isJpeg (0x${compressedBytes[0].toString(16)} 0x${compressedBytes[1].toString(16)})")

            if (!isJpeg) {
                Log.e(TAG, "Generated file is not a valid JPEG!")
                throw IllegalStateException("Failed to create valid JPEG image")
            }
        } else {
            throw IllegalStateException("Compressed data too small")
        }

        return compressedBytes
    }

    private fun uploadFaceImage(bitmap: Bitmap) {
        binding.progressBar.visibility = View.VISIBLE
        binding.btnCapture.isEnabled = false
        binding.tvStatus.text = "Compressing..."

        lifecycleScope.launch {
            try {
                // Compress and resize image to ensure it's under 1MB
                val compressedBytes = compressImageUnder1MB(bitmap)

                val fileSizeKB = compressedBytes.size / 1024
                Log.d(TAG, "Compressed image: $fileSizeKB KB, first 10 bytes: ${compressedBytes.take(10).joinToString(" ") { "0x%02x".format(it) }}")

                binding.tvStatus.text = "Uploading... ($fileSizeKB KB)"

                // Create multipart body with JPEG format
                val requestBody = compressedBytes.toRequestBody("image/jpeg".toMediaTypeOrNull())
                val filename = "face_${System.currentTimeMillis()}.jpg"
                val imagePart = MultipartBody.Part.createFormData(
                    "image",
                    filename,
                    requestBody
                )

                Log.d(TAG, "Sending multipart: filename=$filename, size=${compressedBytes.size} bytes")

                // API call
                val response = RetrofitClient.authService.registerFace(imagePart)

                Log.d(TAG, "API response code: ${response.code()}")

                if (response.isSuccessful && response.body() != null) {
                    val result = response.body()!!

                    Log.d(TAG, "Registration successful: userId=${result.userId}, faceId=${result.faceId}, nfcUid=${result.nfcUid}")

                    // Save to preferences including NFC UID
                    prefManager.userId = result.userId
                    prefManager.faceId = result.faceId
                    prefManager.nfcUid = result.nfcUid

                    binding.tvStatus.text = "Success!\nUser ID: ${result.userId}\nNFC UID: ${result.nfcUid}"
                    Toast.makeText(this@FaceRegisterActivity, "Face & NFC registered!", Toast.LENGTH_LONG).show()

                    // Return to MainActivity
                    finish()
                } else {
                    val errorMsg = response.errorBody()?.string() ?: "Unknown error"
                    val errorCode = response.code()

                    Log.e(TAG, "API error - Code: $errorCode, Message: $errorMsg")
                    Log.e(TAG, "Response headers: ${response.headers()}")

                    binding.tvStatus.text = "Failed ($errorCode): ${errorMsg.take(100)}"
                    Toast.makeText(this@FaceRegisterActivity, "Registration failed: $errorCode", Toast.LENGTH_LONG).show()
                }

            } catch (e: Exception) {
                binding.tvStatus.text = "Error: ${e.message}"
                Toast.makeText(this@FaceRegisterActivity, "Network error", Toast.LENGTH_SHORT).show()
                Log.e(TAG, "Upload failed", e)
            } finally {
                binding.progressBar.visibility = View.GONE
                binding.btnCapture.isEnabled = true
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        cameraExecutor.shutdown()
    }

    companion object {
        private const val TAG = "FaceRegisterActivity"
    }
}
