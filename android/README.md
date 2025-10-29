# MyDrive3DX - Android Application

## 📱 프로젝트 개요

MyDrive3DX는 3D 바디스캔 기술을 활용한 차세대 차량 공유 플랫폼입니다. 사용자의 신체 치수를 3D 스캔하여 차량 세팅을 자동으로 최적화하고, AI 기반 졸음 감지 시스템으로 안전한 운전을 지원합니다.

### 주요 기능

- 🔐 **Multi-Factor Authentication (MFA)**: 얼굴 인식 + NFC + BLE 통합 인증
- 🚗 **스마트 차량 대여**: 실시간 차량 조회 및 예약 시스템
- 📱 **원격 차량 제어**: 도어 잠금, 시동, 에어컨 등 원격 제어
- 🎯 **3D 바디스캔**: ML Kit 기반 신체 측정 및 최적 차량 세팅 자동 생성
- 😴 **졸음 감지**: 실시간 운전자 모니터링 및 경고 시스템
- 🎨 **Modern UI**: Jetpack Compose + Material3 디자인

## 🏗️ 기술 스택

### 핵심 기술
- **언어**: Kotlin 1.9.0
- **UI Framework**: Jetpack Compose + Material3
- **아키텍처**: Clean Architecture + MVVM
- **DI**: Hilt (Dagger)
- **비동기**: Coroutines + Flow

### 주요 라이브러리
- **ML Kit**: Face Detection (16.1.7), Pose Detection (18.0.0-beta5)
- **CameraX**: 카메라 미리보기 및 이미지 분석
- **Retrofit2**: REST API 통신
- **Room**: 로컬 데이터베이스
- **DataStore**: Key-Value 저장소
- **Coil**: 이미지 로딩

## 📂 프로젝트 구조

```
android/
├── app/
│   ├── src/main/java/com/hypermob/mydrive3dx/
│   │   ├── data/              # Data Layer
│   │   │   ├── api/           # API 인터페이스
│   │   │   ├── model/         # DTO 모델
│   │   │   ├── repository/    # Repository 구현
│   │   │   │   ├── fake/      # Fake/Mock Repository (Debug)
│   │   │   │   └── ...        # Real Repository (Release)
│   │   │   ├── local/         # Room Database
│   │   │   └── hardware/      # 하드웨어 매니저
│   │   │
│   │   ├── domain/            # Domain Layer
│   │   │   ├── model/         # 도메인 모델
│   │   │   ├── repository/    # Repository 인터페이스
│   │   │   └── usecase/       # Use Cases
│   │   │
│   │   ├── presentation/      # Presentation Layer
│   │   │   ├── ui/            # Composable UI
│   │   │   │   ├── auth/      # 인증 화면
│   │   │   │   ├── rental/    # 대여 화면
│   │   │   │   ├── control/   # 차량 제어 화면
│   │   │   │   ├── bodyscan/  # 바디스캔 화면
│   │   │   │   └── ...
│   │   │   └── viewmodel/     # ViewModels
│   │   │
│   │   ├── di/                # Dependency Injection
│   │   │   ├── AppModule.kt
│   │   │   ├── NetworkModule.kt
│   │   │   ├── RepositoryModule.kt
│   │   │   └── HardwareModule.kt
│   │   │
│   │   └── util/              # 유틸리티
│   │
│   └── build.gradle.kts       # 앱 빌드 설정
│
├── gradle/                     # Gradle 설정
└── build.gradle.kts           # 프로젝트 빌드 설정
```

## 🎯 Clean Architecture

### 계층 구조

```
┌─────────────────────────────────┐
│      Presentation Layer         │
│  (UI, ViewModel, Composables)   │
├─────────────────────────────────┤
│        Domain Layer             │
│  (Use Cases, Models, Repos)     │
├─────────────────────────────────┤
│         Data Layer              │
│  (Repository Impl, API, DB)     │
└─────────────────────────────────┘
```

### 데이터 흐름

```
UI → ViewModel → UseCase → Repository → API/DB
                    ↓
                  Model
                    ↓
                Flow<Result<T>>
```

## 🚀 빠른 시작

### 필수 요구사항

- Android Studio Hedgehog (2023.1.1) 이상
- JDK 17
- Android SDK 34
- Gradle 8.2

### 설치 및 실행

1. **프로젝트 클론**
   ```bash
   git clone <repository-url>
   cd Platform-Application/android
   ```

2. **Android Studio에서 열기**
   - File → Open → android 폴더 선택

3. **Gradle Sync**
   - Android Studio가 자동으로 의존성 다운로드

4. **앱 실행**
   ```bash
   # Debug 빌드 (Fake 데이터 사용)
   ./gradlew installDebug

   # Release 빌드 (실제 API 사용)
   ./gradlew assembleRelease
   ```

## 🔧 빌드 설정

### Debug vs Release

#### Debug 빌드
- Fake Repository 사용 (API 없이 테스트 가능)
- 더미 데이터로 모든 기능 테스트
- 네트워크 지연 시뮬레이션

#### Release 빌드
- 실제 API 연결
- 프로덕션 환경 설정
- 코드 난독화 적용

### 환경 설정

**NetworkModule.kt**에서 API URL 설정:

```kotlin
const val BASE_URL = "https://api.mydrive3dx.com/api/"
```

## 📱 주요 기능 상세

### 1. Multi-Factor Authentication (MFA)

3단계 인증 시스템:

```kotlin
// 1. 얼굴 인식
FaceDetectionManager → ML Kit Face Detection

// 2. NFC 인증
NFCManager → Android NFC API

// 3. BLE 인증
BLEManager → Bluetooth Low Energy
```

### 2. 3D 바디스캔

7가지 신체 치수 측정:

- Sitting Height (앉은키)
- Shoulder Width (어깨 너비)
- Arm Length (팔 길이)
- Head Height (머리 높이)
- Eye Height (눈 높이)
- Leg Length (다리 길이)
- Torso Length (상체 길이)

### 3. 차량 자동 세팅

8축 자동 조정:

- Seat Front/Back (시트 전후)
- Seat Up/Down (시트 상하)
- Seat Back Angle (등받이 각도)
- Headrest Height (머리 받침 높이)
- Left/Right Mirror Angle (사이드 미러)
- Steering Wheel Height/Depth (핸들 높이/깊이)

### 4. 원격 차량 제어

- 🔒 도어 잠금/해제
- 🔑 시동 켜기/끄기
- ❄️ 에어컨 제어
- 📍 차량 위치 확인
- 🔊 경적/라이트 제어

## 🧪 테스트

### Fake Data 시스템

Debug 모드에서는 Fake Repository가 자동으로 사용됩니다:

```kotlin
// DummyData.kt
object DummyData {
    val dummyUser = User(...)
    val dummyVehicles = listOf(...)
    val dummyRentals = listOf(...)
}

// FakeAuthRepositoryImpl.kt
override suspend fun login(request: LoginRequest): Flow<Result<AuthToken>> = flow {
    emit(Result.Loading)
    delay(500) // 네트워크 시뮬레이션
    emit(Result.Success(DummyData.dummyAuthToken))
}
```

### 테스트 계정

```
Email: test@hypermob.com
Password: (아무 값이나 입력)
```

## 📊 상태 관리

### Result 패턴

```kotlin
sealed class Result<out T> {
    data class Success<T>(val data: T) : Result<T>()
    data class Error(val exception: Exception) : Result<Nothing>()
    object Loading : Result<Nothing>()
}
```

### Flow 기반 데이터 스트림

```kotlin
// Repository
override suspend fun getVehicles(): Flow<Result<List<Vehicle>>> = flow {
    emit(Result.Loading)
    try {
        val vehicles = api.getVehicles()
        emit(Result.Success(vehicles))
    } catch (e: Exception) {
        emit(Result.Error(e))
    }
}

// ViewModel
viewModelScope.launch {
    repository.getVehicles().collect { result ->
        when (result) {
            is Result.Loading -> _uiState.value = UiState.Loading
            is Result.Success -> _uiState.value = UiState.Success(result.data)
            is Result.Error -> _uiState.value = UiState.Error(result.exception.message)
        }
    }
}
```

## 🔐 보안

### Token 관리

```kotlin
// TokenManager
class TokenManager @Inject constructor(
    private val dataStore: DataStore<Preferences>
) {
    suspend fun saveToken(token: AuthToken)
    suspend fun getAccessToken(): String?
    suspend fun clearTokens()
}
```

### API 인증

```kotlin
// AuthInterceptor
class AuthInterceptor @Inject constructor(
    private val tokenManager: TokenManager
) : Interceptor {
    override fun intercept(chain: Chain): Response {
        val token = tokenManager.getAccessToken()
        val request = chain.request().newBuilder()
            .addHeader("Authorization", "Bearer $token")
            .build()
        return chain.proceed(request)
    }
}
```

## 📈 성능 최적화

### 이미지 로딩

```kotlin
// Coil with caching
AsyncImage(
    model = ImageRequest.Builder(LocalContext.current)
        .data(imageUrl)
        .crossfade(true)
        .memoryCachePolicy(CachePolicy.ENABLED)
        .diskCachePolicy(CachePolicy.ENABLED)
        .build(),
    contentDescription = null
)
```

### 데이터베이스 캐싱

```kotlin
// Room + Repository pattern
override suspend fun getVehicles(): Flow<Result<List<Vehicle>>> = flow {
    // 1. 로컬 캐시 먼저 확인
    val cachedVehicles = vehicleDao.getAllVehicles()
    if (cachedVehicles.isNotEmpty()) {
        emit(Result.Success(cachedVehicles.map { it.toDomain() }))
    }

    // 2. API 호출
    try {
        val apiVehicles = api.getVehicles()
        vehicleDao.insertAll(apiVehicles.map { it.toEntity() })
        emit(Result.Success(apiVehicles.map { it.toDomain() }))
    } catch (e: Exception) {
        if (cachedVehicles.isEmpty()) {
            emit(Result.Error(e))
        }
    }
}
```

## 🐛 디버깅

### Logcat 필터

```
Tag: MyDrive3DX
```

### 주요 로그 포인트

- API 호출: `NetworkModule`
- 인증 흐름: `AuthViewModel`
- 바디스캔: `BodyScanManager`
- 차량 제어: `VehicleControlViewModel`

## 📚 참고 문서

- [Architecture Guide](ARCHITECTURE.md) - 아키텍처 상세 설명
- [Features Documentation](FEATURES.md) - 기능별 상세 가이드
- [Setup Guide](SETUP_GUIDE.md) - 개발 환경 설정
- [API Development Guide](../API_DEVELOPMENT_GUIDE.md) - 백엔드 API 명세

## 🤝 기여 가이드

### 코드 컨벤션

- Kotlin Coding Conventions 준수
- 클래스명: PascalCase
- 함수/변수명: camelCase
- 상수: UPPER_SNAKE_CASE

### 커밋 메시지

```
feat: 새로운 기능 추가
fix: 버그 수정
docs: 문서 수정
refactor: 코드 리팩토링
test: 테스트 추가
style: 코드 포맷팅
```

## 📝 라이선스

Copyright © 2024 HYPERMOB

## 📞 문의

프로젝트 관련 문의사항은 개발팀에게 연락하세요.

---

**Last Updated**: 2025-10-03
**Version**: 1.0.0
