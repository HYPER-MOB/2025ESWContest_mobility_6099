# 🐛 Face Registration 디버깅 가이드

## 문제: "Image must be in JPEG format" 에러

### 1단계: Lambda 함수 업데이트

#### A. Lambda Layer 업데이트
```powershell
# AWS Console에서
1. Lambda → Layers → hypermob-mfa-shared-layer
2. Create version 클릭
3. lambda/lambda-layer.zip 업로드
4. Create 클릭
```

#### B. auth-face Lambda 함수 업데이트
```powershell
# AWS Console에서
1. Lambda → hypermob-mfa-auth-face
2. Code 탭 → Upload from → .zip file
3. lambda/lambda-auth-face.zip 업로드
4. Save 클릭
```

---

### 2단계: Android 앱 업데이트 및 테스트

```bash
# APK 설치
adb install -r android_mvp/app/build/outputs/apk/debug/app-debug.apk

# Logcat 모니터링
adb logcat | grep -E "(FaceRegisterActivity|OkHttp)"
```

---

### 3단계: 로그 분석

#### Android 앱 로그 (예상)

✅ **정상 동작 시:**
```
D/FaceRegisterActivity: Bitmap created: 1920x1440
D/FaceRegisterActivity: Resized to 1920x1440
D/FaceRegisterActivity: Compressed at quality 90: 750 KB
D/FaceRegisterActivity: JPEG header check: true (0xff 0xd8)
D/FaceRegisterActivity: Compressed image: 750 KB, first 10 bytes: 0xff 0xd8 0xff 0xe0 0x00 0x10 0x4a 0x46 0x49 0x46
D/FaceRegisterActivity: Sending multipart: filename=face_1234567890.jpg, size=768000 bytes
D/FaceRegisterActivity: API response code: 200
D/FaceRegisterActivity: Registration successful: userId=U12345ABCDEF, faceId=F1A2B3C4D5E6F708
```

❌ **JPEG 에러 시:**
```
D/FaceRegisterActivity: JPEG header check: true (0xff 0xd8)
D/FaceRegisterActivity: Compressed image: 750 KB, first 10 bytes: 0xff 0xd8 0xff 0xe0 ...
D/FaceRegisterActivity: API response code: 400
E/FaceRegisterActivity: API error - Code: 400, Message: {"error":"invalid_image","message":"Image must be in JPEG format (got: 0x?? 0x??)"}
```

#### Lambda CloudWatch 로그 (예상)

✅ **정상 동작 시:**
```json
INFO Content-Type received { contentType: 'multipart/form-data; boundary=...' }
INFO Request encoding { isBase64Encoded: true, bodyLength: 1024000 }
INFO Body parsed { bodyLength: 768000 }
INFO Files parsed { hasImage: true, fileCount: 1 }
INFO Image data extracted {
  dataLength: 750000,
  isBuffer: true,
  firstBytes: 'ffd8ffe000104a46494600'
}
INFO JPEG validation { byte0: 'ff', byte1: 'd8', isJpeg: true }
INFO Generated IDs { userId: 'U12345ABCDEF', faceId: 'F1A2B3C4D5E6F708' }
INFO Face registration successful
```

❌ **JPEG 에러 시:**
```json
INFO Image data extracted {
  dataLength: 750000,
  isBuffer: true,
  firstBytes: '????????????????????????'  ← 이게 문제!
}
INFO JPEG validation { byte0: '??', byte1: '??', isJpeg: false }
ERROR 400: invalid_image - Image must be in JPEG format (got: 0x?? 0x??)
```

---

### 4단계: 문제별 해결 방법

#### 문제 A: Lambda에서 첫 바이트가 잘못됨

**증상:**
- Android: `0xff 0xd8` ✅
- Lambda: `0x?? 0x??` ❌

**원인:** API Gateway의 바이너리 미디어 타입 설정 문제

**해결:**
```powershell
# AWS Console
1. API Gateway → HYPERMOB MFA Authentication API
2. Settings → Binary Media Types
3. Add Binary Media Type: multipart/form-data
4. Save
5. Actions → Deploy API → stage: v1
```

#### 문제 B: Lambda에서 파일이 파싱되지 않음

**증상:**
```
ERROR 400: missing_image - Image file is required
```

**원인:** multipart boundary 파싱 실패

**해결:** Retrofit OkHttp 로깅 활성화하여 실제 전송 데이터 확인

`RetrofitClient.kt` 수정:
```kotlin
private val loggingInterceptor = HttpLoggingInterceptor().apply {
    level = HttpLoggingInterceptor.Level.HEADERS  // BODY로 변경
}
```

#### 문제 C: 이미지가 너무 큼

**증상:**
```
ERROR 400: file_too_large - Image size must not exceed 1MB
```

**해결:** 이미 구현됨 - `compressImageUnder1MB()`가 자동 처리

---

### 5단계: API Gateway 바이너리 설정 확인

#### 필수 설정:

1. **Binary Media Types**
   - Settings에서 추가: `multipart/form-data`
   - 또는: `*/*` (모든 바이너리 허용)

2. **Integration Request**
   - Method: POST
   - Integration type: Lambda Function
   - ✅ **Use Lambda Proxy integration** 체크

3. **Deploy API**
   - 설정 변경 후 반드시 재배포 필요!

---

### 6단계: 최종 체크리스트

업데이트 전:
- [ ] Lambda Layer 업데이트 완료
- [ ] Lambda auth-face 함수 업데이트 완료
- [ ] API Gateway Binary Media Types 설정 완료
- [ ] API Gateway 재배포 완료
- [ ] Android APK 설치 완료

테스트:
- [ ] Logcat에서 `JPEG header check: true` 확인
- [ ] Logcat에서 `first 10 bytes: 0xff 0xd8 ...` 확인
- [ ] CloudWatch에서 `isJpeg: true` 확인
- [ ] 응답 코드 200 확인
- [ ] user_id 저장 확인

---

## 🚨 여전히 실패하는 경우

### 임시 해결책: Base64 전송

Lambda `auth-face/index.js` 수정:
```javascript
// JPEG 검증 전에 추가
log('INFO', 'Raw image data sample', {
  firstTenBytes: Array.from(imageData.slice(0, 10)),
  asHex: Array.from(imageData.slice(0, 10)).map(b => b.toString(16).padStart(2, '0')).join(' ')
});
```

Android 앱 대안 방법 (필요시):
```kotlin
// Base64로 인코딩해서 전송
val base64Image = Base64.encodeToString(compressedBytes, Base64.NO_WRAP)
val json = JSONObject().apply {
    put("image", base64Image)
}
// POST 요청으로 전송
```

---

## 📞 추가 도움말

문제가 계속되면 다음 정보를 수집하세요:

1. **Android Logcat 전체 로그**
   ```bash
   adb logcat -d > android_log.txt
   ```

2. **Lambda CloudWatch 로그**
   - 최근 15분 로그 전체 복사

3. **API Gateway 설정 스크린샷**
   - Binary Media Types
   - Integration Request
   - Method Request

이 정보로 정확한 진단이 가능합니다!
