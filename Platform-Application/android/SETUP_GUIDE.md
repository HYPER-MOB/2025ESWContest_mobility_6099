# MyDrive3DX - Setup & Build Guide

## 목차

1. [개발 환경 설정](#개발-환경-설정)
2. [프로젝트 설정](#프로젝트-설정)
3. [빌드 설정](#빌드-설정)
4. [실행 및 디버깅](#실행-및-디버깅)
5. [문제 해결](#문제-해결)
6. [배포](#배포)

---

## 개발 환경 설정

### 필수 요구사항

#### 1. Android Studio

**권장 버전**: Hedgehog (2023.1.1) 이상

**다운로드**: [Android Studio](https://developer.android.com/studio)

**필수 플러그인**:
- Kotlin Plugin (기본 포함)
- Android Gradle Plugin

**설치 확인**:
```bash
# Android Studio 버전 확인
# Help → About Android Studio
```

#### 2. JDK (Java Development Kit)

**필수 버전**: JDK 17

**설치 (Windows)**:
```bash
# Chocolatey 사용
choco install openjdk17

# 또는 수동 다운로드
# https://adoptium.net/
```

**설치 (macOS)**:
```bash
# Homebrew 사용
brew install openjdk@17

# 환경 변수 설정
echo 'export PATH="/opt/homebrew/opt/openjdk@17/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

**설치 (Linux)**:
```bash
sudo apt-get update
sudo apt-get install openjdk-17-jdk
```

**설치 확인**:
```bash
java -version
# openjdk version "17.x.x" 출력 확인
```

#### 3. Android SDK

**필수 SDK**:
- SDK Platform: Android 14.0 (API 34)
- SDK Build-Tools: 34.0.0
- Android Emulator
- Android SDK Platform-Tools

**Android Studio에서 설치**:
```
Tools → SDK Manager → SDK Platforms
- [✓] Android 14.0 (API 34)

Tools → SDK Manager → SDK Tools
- [✓] Android SDK Build-Tools 34
- [✓] Android Emulator
- [✓] Android SDK Platform-Tools
```

#### 4. Gradle

**사용 버전**: 8.2 (프로젝트에 포함됨)

**Gradle Wrapper 사용** (권장):
```bash
# Windows
gradlew.bat --version

# macOS/Linux
./gradlew --version
```

### 선택 사항

#### Git

**설치**:
```bash
# Windows (Chocolatey)
choco install git

# macOS (Homebrew)
brew install git

# Linux
sudo apt-get install git
```

**설정**:
```bash
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"
```

---

## 프로젝트 설정

### 1. 프로젝트 Clone

```bash
# Git Clone
git clone <repository-url>
cd Platform-Application/android
```

### 2. Android Studio에서 프로젝트 열기

1. **Android Studio 실행**
2. **File → Open**
3. **`android` 폴더 선택**
4. **Open** 클릭

### 3. Gradle Sync

프로젝트를 열면 자동으로 Gradle Sync가 실행됩니다.

**수동 실행**:
```
File → Sync Project with Gradle Files
```

**또는 터미널에서**:
```bash
./gradlew build --refresh-dependencies
```

### 4. 의존성 다운로드

Gradle Sync 중 자동으로 다운로드됩니다.

**주요 의존성**:
- Jetpack Compose
- Hilt
- Retrofit
- Room
- ML Kit
- CameraX
- Coil

**수동 확인**:
```bash
./gradlew dependencies
```

### 5. 환경 변수 설정 (선택)

**local.properties 생성**:
```properties
# android/local.properties

sdk.dir=/path/to/Android/Sdk

# API Key (필요한 경우)
API_KEY=your_api_key_here
MAPS_API_KEY=your_maps_api_key_here
```

**⚠️ 주의**: `local.properties`는 `.gitignore`에 포함되어 있어 Git에 커밋되지 않습니다.

---

## 빌드 설정

### Build Variants

프로젝트는 두 가지 빌드 변형을 제공합니다:

#### 1. Debug Build

**특징**:
- Fake Repository 사용 (API 불필요)
- 로깅 활성화
- 디버깅 가능
- 난독화 없음

**빌드 명령**:
```bash
# APK 생성
./gradlew assembleDebug

# 설치
./gradlew installDebug

# 빌드 + 설치 + 실행
./gradlew installDebug && adb shell am start -n com.hypermob.mydrive3dx/.MainActivity
```

**Android Studio에서**:
```
Build → Select Build Variant → debug
Run → Run 'app'
```

#### 2. Release Build

**특징**:
- 실제 API 연결
- 로깅 비활성화
- ProGuard/R8 난독화
- 서명 필요

**빌드 명령**:
```bash
./gradlew assembleRelease
```

**서명된 APK 생성**:
```bash
./gradlew bundleRelease
```

### Build Types 설정

**build.gradle.kts (app)**:
```kotlin
android {
    buildTypes {
        debug {
            isMinifyEnabled = false
            isDebuggable = true
            // Fake Repository 사용
        }

        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            // 실제 API 사용
        }
    }
}
```

### ProGuard 설정

**proguard-rules.pro**:
```proguard
# Retrofit
-keepattributes Signature
-keepattributes *Annotation*
-keep class retrofit2.** { *; }

# Gson
-keep class com.google.gson.** { *; }
-keep class * implements com.google.gson.TypeAdapter
-keep class * implements com.google.gson.TypeAdapterFactory
-keep class * implements com.google.gson.JsonSerializer
-keep class * implements com.google.gson.JsonDeserializer

# Domain Models
-keep class com.hypermob.mydrive3dx.domain.model.** { *; }

# ML Kit
-keep class com.google.mlkit.** { *; }
```

---

## 실행 및 디버깅

### 에뮬레이터 설정

#### 1. AVD (Android Virtual Device) 생성

**Android Studio에서**:
```
Tools → Device Manager → Create Device

1. 디바이스 선택: Pixel 6 (권장)
2. 시스템 이미지: Android 14.0 (API 34)
3. AVD Name: MyDrive_Emulator
4. Finish
```

**필수 설정**:
- RAM: 최소 2GB (권장 4GB)
- 내부 저장소: 최소 2GB
- Camera: Webcam (얼굴 인식 테스트용)

#### 2. 에뮬레이터 실행

**Android Studio에서**:
```
Device Manager → MyDrive_Emulator → Play 버튼
```

**명령줄에서**:
```bash
# 에뮬레이터 목록 확인
emulator -list-avds

# 에뮬레이터 실행
emulator -avd MyDrive_Emulator
```

### 실제 기기 연결

#### 1. USB 디버깅 활성화

**Android 기기에서**:
```
설정 → 휴대전화 정보 → 빌드 번호 7회 탭
설정 → 개발자 옵션 → USB 디버깅 활성화
```

#### 2. 기기 연결 확인

```bash
# 연결된 기기 확인
adb devices

# 출력 예시:
# List of devices attached
# ABC123456789    device
```

### 앱 실행

#### Android Studio에서 실행

1. **Build Variant 선택**
   ```
   View → Tool Windows → Build Variants
   debug 선택
   ```

2. **디바이스 선택**
   ```
   상단 툴바 → 디바이스 드롭다운 → 에뮬레이터/기기 선택
   ```

3. **실행**
   ```
   Run → Run 'app' (Shift + F10)
   ```

#### 명령줄에서 실행

```bash
# Debug APK 빌드 및 설치
./gradlew installDebug

# 앱 실행
adb shell am start -n com.hypermob.mydrive3dx/.MainActivity

# 한 번에 실행
./gradlew installDebug && adb shell am start -n com.hypermob.mydrive3dx/.MainActivity
```

### 디버깅

#### 1. Logcat 확인

**Android Studio에서**:
```
View → Tool Windows → Logcat

필터 설정:
- Package: com.hypermob.mydrive3dx
- Log Level: Debug
```

**명령줄에서**:
```bash
# 전체 로그
adb logcat

# 앱 로그만 필터
adb logcat | grep "MyDrive3DX"

# 특정 태그 필터
adb logcat -s AuthViewModel
```

#### 2. Breakpoint 설정

1. 코드 라인 번호 옆 클릭
2. Debug 모드로 실행: `Run → Debug 'app'`
3. Breakpoint에서 멈춤
4. Variables 창에서 변수 확인

#### 3. Database Inspector

**Room 데이터베이스 확인**:
```
View → Tool Windows → App Inspection → Database Inspector
```

#### 4. Network Inspector

**API 호출 확인**:
```
View → Tool Windows → App Inspection → Network Inspector
```

---

## 문제 해결

### 자주 발생하는 문제

#### 1. Gradle Sync 실패

**증상**:
```
Could not resolve all dependencies
```

**해결**:
```bash
# Gradle 캐시 정리
./gradlew clean

# 의존성 재다운로드
./gradlew build --refresh-dependencies

# Gradle Wrapper 재설치
./gradlew wrapper --gradle-version 8.2
```

#### 2. SDK 버전 불일치

**증상**:
```
The SDK directory does not exist
```

**해결**:
```bash
# local.properties 생성/수정
echo "sdk.dir=/path/to/Android/Sdk" > local.properties

# Windows 예시
echo "sdk.dir=C:\\Users\\YourName\\AppData\\Local\\Android\\Sdk" > local.properties

# macOS/Linux 예시
echo "sdk.dir=/Users/YourName/Library/Android/sdk" > local.properties
```

#### 3. Kotlin 버전 불일치

**증상**:
```
Module was compiled with an incompatible version of Kotlin
```

**해결**:
```bash
# 프로젝트 clean
./gradlew clean

# Gradle 캐시 정리
rm -rf ~/.gradle/caches/

# 프로젝트 재빌드
./gradlew build
```

#### 4. ML Kit 의존성 오류

**증상**:
```
Could not find com.google.mlkit:face-detection
```

**해결**:

**build.gradle.kts (project)**에 추가:
```kotlin
allprojects {
    repositories {
        google()
        mavenCentral()
    }
}
```

#### 5. CameraX 권한 오류

**증상**:
```
Permission Denial: opening camera requires android.permission.CAMERA
```

**해결**:

**AndroidManifest.xml** 확인:
```xml
<uses-permission android:name="android.permission.CAMERA" />
<uses-feature android:name="android.hardware.camera" />
```

**Runtime Permission 요청** 확인:
```kotlin
// MainActivity.kt
val cameraPermission = Manifest.permission.CAMERA

val launcher = rememberLauncherForActivityResult(
    ActivityResultContracts.RequestPermission()
) { isGranted ->
    if (isGranted) {
        // 카메라 사용
    }
}

LaunchedEffect(Unit) {
    launcher.launch(cameraPermission)
}
```

#### 6. Hilt 주입 오류

**증상**:
```
Dagger does not support injection into private fields
```

**해결**:

필드를 `public` 또는 `internal`로 변경:
```kotlin
// ❌ 잘못된 코드
private lateinit var repository: Repository

// ✅ 올바른 코드
@Inject
lateinit var repository: Repository
```

**Application 클래스 확인**:
```kotlin
@HiltAndroidApp
class MyDrive3DXApplication : Application()
```

**AndroidManifest.xml 확인**:
```xml
<application
    android:name=".MyDrive3DXApplication"
    ...>
```

#### 7. 빌드 속도 개선

**gradle.properties 최적화**:
```properties
# Gradle 데몬
org.gradle.daemon=true

# 병렬 빌드
org.gradle.parallel=true

# JVM 힙 메모리
org.gradle.jvmargs=-Xmx4096m -XX:MaxMetaspaceSize=512m

# Kotlin 증분 컴파일
kotlin.incremental=true

# R8 최적화
android.enableR8.fullMode=true
```

---

## 배포

### Google Play Store 배포

#### 1. 서명 키 생성

```bash
keytool -genkey -v \
  -keystore mydrive3dx-release.jks \
  -alias mydrive3dx \
  -keyalg RSA \
  -keysize 2048 \
  -validity 10000
```

**정보 입력**:
- 비밀번호
- 이름
- 조직
- 도시/국가

#### 2. 서명 설정

**gradle.properties** (프로젝트 루트):
```properties
RELEASE_STORE_FILE=../mydrive3dx-release.jks
RELEASE_STORE_PASSWORD=your_store_password
RELEASE_KEY_ALIAS=mydrive3dx
RELEASE_KEY_PASSWORD=your_key_password
```

**build.gradle.kts (app)**:
```kotlin
android {
    signingConfigs {
        create("release") {
            storeFile = file(project.property("RELEASE_STORE_FILE") as String)
            storePassword = project.property("RELEASE_STORE_PASSWORD") as String
            keyAlias = project.property("RELEASE_KEY_ALIAS") as String
            keyPassword = project.property("RELEASE_KEY_PASSWORD") as String
        }
    }

    buildTypes {
        release {
            signingConfig = signingConfigs.getByName("release")
            // ...
        }
    }
}
```

#### 3. AAB (Android App Bundle) 생성

```bash
./gradlew bundleRelease
```

**출력 위치**:
```
app/build/outputs/bundle/release/app-release.aab
```

#### 4. Google Play Console 업로드

1. [Google Play Console](https://play.google.com/console) 접속
2. 앱 선택 또는 생성
3. 프로덕션 → 새 버전 만들기
4. AAB 파일 업로드
5. 출시 노트 작성
6. 검토 후 출시

### APK 직접 배포

#### 서명된 APK 생성

```bash
./gradlew assembleRelease
```

**출력 위치**:
```
app/build/outputs/apk/release/app-release.apk
```

#### APK 설치

```bash
# USB 연결된 기기에 설치
adb install app/build/outputs/apk/release/app-release.apk

# 기존 앱 덮어쓰기
adb install -r app/build/outputs/apk/release/app-release.apk
```

---

## 개발 팁

### 유용한 Gradle 명령어

```bash
# 빌드 캐시 정리
./gradlew clean

# 전체 빌드
./gradlew build

# 테스트 실행
./gradlew test

# Lint 검사
./gradlew lint

# 의존성 트리 확인
./gradlew dependencies

# 빌드 속도 분석
./gradlew build --scan
```

### Android Studio 단축키

**Windows/Linux**:
- 빌드: `Ctrl + F9`
- 실행: `Shift + F10`
- 디버그: `Shift + F9`
- 파일 찾기: `Ctrl + Shift + N`
- 클래스 찾기: `Ctrl + N`
- 코드 정렬: `Ctrl + Alt + L`

**macOS**:
- 빌드: `Cmd + F9`
- 실행: `Ctrl + R`
- 디버그: `Ctrl + D`
- 파일 찾기: `Cmd + Shift + O`
- 클래스 찾기: `Cmd + O`
- 코드 정렬: `Cmd + Option + L`

### 디버깅 팁

1. **Logcat 필터 활용**
   ```kotlin
   private val TAG = "MyViewModel"
   Log.d(TAG, "Message: $data")
   ```

2. **Timber 사용** (권장)
   ```kotlin
   // Application.kt
   if (BuildConfig.DEBUG) {
       Timber.plant(Timber.DebugTree())
   }

   // 사용
   Timber.d("Debug message")
   Timber.e(exception, "Error occurred")
   ```

3. **Compose Preview 활용**
   ```kotlin
   @Preview(showBackground = true)
   @Composable
   fun LoginScreenPreview() {
       MyDrive3DXTheme {
           LoginScreen(
               viewModel = previewViewModel(),
               onLoginSuccess = {}
           )
       }
   }
   ```

---

## 추가 리소스

### 공식 문서

- [Android Developers](https://developer.android.com/)
- [Jetpack Compose](https://developer.android.com/jetpack/compose)
- [Kotlin](https://kotlinlang.org/docs/)
- [Hilt](https://dagger.dev/hilt/)
- [ML Kit](https://developers.google.com/ml-kit)

### 도구

- [Postman](https://www.postman.com/) - API 테스트
- [Charles Proxy](https://www.charlesproxy.com/) - 네트워크 디버깅
- [Scrcpy](https://github.com/Genymobile/scrcpy) - 화면 미러링
- [ADB Idea](https://plugins.jetbrains.com/plugin/7380-adb-idea) - ADB 플러그인

### 학습 자료

- [Android Basics with Compose](https://developer.android.com/courses/android-basics-compose/course)
- [Clean Architecture Guide](https://blog.cleancoder.com/uncle-bob/2012/08/13/the-clean-architecture.html)
- [Kotlin Coroutines](https://kotlinlang.org/docs/coroutines-guide.html)

---

## 문의 및 지원

문제가 해결되지 않을 경우:

1. **GitHub Issues** 확인
2. **프로젝트 문서** 재확인
3. **개발팀에 문의**

---

**Last Updated**: 2025-10-03
