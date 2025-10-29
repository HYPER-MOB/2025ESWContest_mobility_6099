# JPEG Format Issue - Root Cause and Fix

## Problem Summary

User reported receiving error: `"Image must be in JPEG format"` from Lambda, even though the Android app claimed to be sending JPEG.

**Critical clue**: Lambda logs showed image bytes starting with `0xfd 0xfd` instead of JPEG header `0xff 0xd8`

User's question: "png을 확장명만 바꿔서 보내고 있는거니?" (Are we just changing PNG extension?)

## Root Cause

The original implementation used **CameraX's `OnImageCapturedCallback`** which returns an `ImageProxy` object:

```kotlin
// OLD CODE - WRONG APPROACH
imageCapture.takePicture(
    ContextCompat.getMainExecutor(this),
    object : ImageCapture.OnImageCapturedCallback() {
        override fun onCaptureSuccess(image: ImageProxy) {
            val bitmap = imageProxyToBitmap(image)  // ❌ Problem here!
            uploadFaceImage(bitmap)
        }
    }
)

private fun imageProxyToBitmap(image: ImageProxy): Bitmap? {
    val buffer = image.planes[0].buffer
    val bytes = ByteArray(buffer.remaining())
    buffer.get(bytes)
    val bitmap = BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
    return bitmap
}
```

### Why This Failed:

1. **ImageProxy format**: `OnImageCapturedCallback` provides `ImageProxy` in **YUV_420_888 format** (camera's raw format), NOT JPEG
2. **Direct buffer reading**: Reading `planes[0].buffer` gives raw YUV data, not an encoded image
3. **BitmapFactory confusion**: `BitmapFactory.decodeByteArray()` tried to decode YUV bytes as if they were an image file
4. **Format mismatch**: The resulting bitmap, when re-compressed with `Bitmap.CompressFormat.JPEG`, may have been creating malformed data or the original YUV bytes were being sent directly

Result: Android app sent `0xfd 0xfd` (corrupted/wrong format) instead of JPEG `0xff 0xd8`

## Solution

### Changed to File-Based Capture

Use **CameraX's `OnImageSavedCallback`** which writes a proper JPEG file:

```kotlin
// NEW CODE - CORRECT APPROACH
val photoFile = File(cacheDir, "temp_face_${System.currentTimeMillis()}.jpg")
val outputOptions = ImageCapture.OutputFileOptions.Builder(photoFile).build()

imageCapture.takePicture(
    outputOptions,
    ContextCompat.getMainExecutor(this),
    object : ImageCapture.OnImageSavedCallback {
        override fun onImageSaved(output: ImageCapture.OutputFileResults) {
            val jpegBytes = photoFile.readBytes()

            // Verify JPEG header
            val byte0 = jpegBytes[0].toInt() and 0xFF
            val byte1 = jpegBytes[1].toInt() and 0xFF

            if (byte0 == 0xFF && byte1 == 0xD8) {  // ✅ Valid JPEG!
                val bitmap = BitmapFactory.decodeByteArray(jpegBytes, 0, jpegBytes.size)
                uploadFaceImage(bitmap)
            }

            photoFile.delete()  // Clean up
        }
    }
)
```

### Why This Works:

1. **CameraX handles encoding**: `OnImageSavedCallback` uses CameraX's internal JPEG encoder
2. **Proper format**: The saved file is guaranteed to be a valid JPEG with correct headers
3. **Header validation**: We verify `0xFF 0xD8` before processing
4. **Clean bitmap**: `BitmapFactory.decodeByteArray()` now receives real JPEG data, creating a valid bitmap
5. **Compression works**: `compressImageUnder1MB()` now compresses an actual bitmap to JPEG properly

## Changes Made

### [FaceRegisterActivity.kt](android_mvp/app/src/main/java/com/hypermob/android_mvp/ui/FaceRegisterActivity.kt)

1. **Import added**: `import java.io.File`

2. **`captureImage()` rewritten**:
   - Changed from `takePicture(executor, OnImageCapturedCallback)`
   - To: `takePicture(outputOptions, executor, OnImageSavedCallback)`
   - Saves to temporary file in cache directory
   - Reads file bytes directly
   - Validates JPEG header before processing
   - Deletes temp file after use

3. **`imageProxyToBitmap()` removed**: No longer needed

4. **`setJpegQuality(95)` added** to ImageCapture.Builder for high-quality output

## Testing

### Build and Install:
```bash
cd android_mvp
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

### Monitor Logs:
```bash
adb logcat | grep -E "(FaceRegisterActivity|OkHttp)"
```

### Expected Logs:

✅ **Correct Output (JPEG)**:
```
D/FaceRegisterActivity: Captured JPEG file: 2048000 bytes, header: 0xff 0xd8
D/FaceRegisterActivity: Bitmap decoded: 1920x1440
D/FaceRegisterActivity: Compressed at quality 90: 750 KB
D/FaceRegisterActivity: JPEG header check: true (0xff 0xd8)
D/FaceRegisterActivity: Compressed image: 750 KB, first 10 bytes: 0xff 0xd8 0xff 0xe0 ...
D/FaceRegisterActivity: API response code: 200
```

❌ **Previous Wrong Output**:
```
D/FaceRegisterActivity: JPEG header check: true (0xff 0xd8)  ← LIE! Android was wrong
D/FaceRegisterActivity: API response code: 400
E/FaceRegisterActivity: Image must be in JPEG format (got: 0xfd 0xfd)  ← Lambda was right
```

## Key Learnings

1. **CameraX has two capture modes**:
   - `OnImageCapturedCallback`: Returns YUV ImageProxy (for ML/processing)
   - `OnImageSavedCallback`: Saves encoded JPEG file (for uploads)

2. **Always validate binary formats**: Don't trust variable names or extensions, check the actual bytes

3. **Magic numbers are important**:
   - JPEG: `0xFF 0xD8` (start), `0xFF 0xD9` (end)
   - PNG: `0x89 0x50 0x4E 0x47`
   - Our wrong data: `0xFD 0xFD` (corrupted/unknown)

4. **When debugging multipart uploads**: Always log both sides (client + server) to see where data gets corrupted

## References

- CameraX Documentation: https://developer.android.com/training/camerax
- JPEG File Format: https://en.wikipedia.org/wiki/JPEG#Syntax_and_structure
- Lambda multipart parser: [lambda/lambda-layer/nodejs/shared/utils.js](../lambda/lambda-layer/nodejs/shared/utils.js)
