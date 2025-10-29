# MyDrive3DX - Architecture Documentation

## 목차

1. [아키텍처 개요](#아키텍처-개요)
2. [Clean Architecture](#clean-architecture)
3. [MVVM 패턴](#mvvm-패턴)
4. [의존성 주입 (Hilt)](#의존성-주입-hilt)
5. [데이터 흐름](#데이터-흐름)
6. [모듈 구조](#모듈-구조)
7. [Repository 패턴](#repository-패턴)
8. [상태 관리](#상태-관리)
9. [에러 처리](#에러-처리)
10. [테스트 전략](#테스트-전략)

---

## 아키텍처 개요

MyDrive3DX는 **Clean Architecture**와 **MVVM 패턴**을 결합한 현대적인 Android 아키텍처를 채택하고 있습니다.

### 핵심 원칙

1. **관심사의 분리 (Separation of Concerns)**
   - 각 계층은 독립적인 책임을 가짐
   - UI 로직과 비즈니스 로직의 명확한 분리

2. **의존성 역전 (Dependency Inversion)**
   - 상위 계층이 하위 계층에 의존하지 않음
   - 인터페이스를 통한 느슨한 결합

3. **단일 책임 원칙 (Single Responsibility)**
   - 각 클래스는 하나의 책임만 가짐
   - 변경의 이유가 하나만 존재

4. **테스트 용이성 (Testability)**
   - Mock/Fake 객체를 통한 쉬운 테스트
   - 각 계층을 독립적으로 테스트 가능

### 전체 구조도

```
┌─────────────────────────────────────────────────────┐
│                 Presentation Layer                   │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────┐  │
│  │    UI    │  │ViewModel │  │  UiState/Event   │  │
│  │(Compose) │◄─┤          │◄─┤                  │  │
│  └──────────┘  └──────────┘  └──────────────────┘  │
└────────────────────────┬────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────┐
│                   Domain Layer                       │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────┐  │
│  │ Use Case │  │  Model   │  │    Repository    │  │
│  │          │  │          │  │   (Interface)    │  │
│  └──────────┘  └──────────┘  └──────────────────┘  │
└────────────────────────┬────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────┐
│                    Data Layer                        │
│  ┌──────────────┐  ┌─────────┐  ┌────────────────┐ │
│  │  Repository  │  │   API   │  │   Database     │ │
│  │     Impl     │  │         │  │   (Room)       │ │
│  └──────────────┘  └─────────┘  └────────────────┘ │
│                                                      │
│  ┌──────────────────────────────────────────────┐  │
│  │           Fake/Mock Repository               │  │
│  │          (Debug Build Only)                  │  │
│  └──────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

---

## Clean Architecture

### 계층별 설명

#### 1. Presentation Layer (프레젠테이션 계층)

**위치**: `presentation/`

**역할**:
- UI 렌더링 및 사용자 상호작용 처리
- ViewModel을 통한 상태 관리
- 화면 전환 및 내비게이션

**주요 컴포넌트**:

```kotlin
// UI (Composable)
@Composable
fun LoginScreen(
    viewModel: AuthViewModel = hiltViewModel(),
    onLoginSuccess: () -> Unit
) {
    val uiState by viewModel.uiState.collectAsState()

    when (uiState) {
        is AuthUiState.Loading -> LoadingIndicator()
        is AuthUiState.Success -> LaunchedEffect(Unit) { onLoginSuccess() }
        is AuthUiState.Error -> ErrorMessage(uiState.message)
        else -> LoginForm(onLogin = viewModel::login)
    }
}

// ViewModel
@HiltViewModel
class AuthViewModel @Inject constructor(
    private val loginUseCase: LoginUseCase
) : ViewModel() {

    private val _uiState = MutableStateFlow<AuthUiState>(AuthUiState.Idle)
    val uiState: StateFlow<AuthUiState> = _uiState.asStateFlow()

    fun login(email: String, password: String) {
        viewModelScope.launch {
            loginUseCase(LoginRequest(email, password))
                .collect { result ->
                    _uiState.value = when (result) {
                        is Result.Loading -> AuthUiState.Loading
                        is Result.Success -> AuthUiState.Success(result.data)
                        is Result.Error -> AuthUiState.Error(result.exception.message)
                    }
                }
        }
    }
}
```

**특징**:
- Jetpack Compose로 선언형 UI 구현
- StateFlow를 통한 반응형 상태 관리
- Hilt를 통한 ViewModel 주입

#### 2. Domain Layer (도메인 계층)

**위치**: `domain/`

**역할**:
- 비즈니스 로직의 핵심 정의
- 플랫폼 독립적인 순수 Kotlin 코드
- 데이터 모델 및 Use Case 정의

**구조**:

```
domain/
├── model/              # 도메인 모델
│   ├── User.kt
│   ├── Vehicle.kt
│   ├── Rental.kt
│   ├── BodyScan.kt
│   └── ...
│
├── repository/         # Repository 인터페이스
│   ├── AuthRepository.kt
│   ├── RentalRepository.kt
│   ├── VehicleControlRepository.kt
│   └── BodyScanRepository.kt
│
└── usecase/           # Use Cases
    ├── auth/
    │   ├── LoginUseCase.kt
    │   ├── RegisterUseCase.kt
    │   └── LogoutUseCase.kt
    ├── rental/
    │   ├── GetRentalsUseCase.kt
    │   ├── CreateRentalUseCase.kt
    │   └── CancelRentalUseCase.kt
    └── ...
```

**Domain Model 예시**:

```kotlin
// domain/model/Rental.kt
data class Rental(
    val rentalId: String,
    val userId: String,
    val vehicleId: String,
    val vehicle: Vehicle,
    val startTime: String,
    val endTime: String,
    val totalPrice: Double,
    val status: RentalStatus,
    val pickupLocation: String,
    val returnLocation: String,
    val createdAt: String,
    val updatedAt: String
)

enum class RentalStatus {
    REQUESTED,  // 대여 요청됨
    APPROVED,   // 대여 승인됨
    PICKED,     // 차량 인수함
    RETURNED,   // 차량 반납함
    CANCELED    // 대여 취소됨
}
```

**Use Case 예시**:

```kotlin
// domain/usecase/rental/GetRentalsUseCase.kt
class GetRentalsUseCase @Inject constructor(
    private val rentalRepository: RentalRepository
) {
    suspend operator fun invoke(
        status: RentalStatus? = null
    ): Flow<Result<List<Rental>>> {
        return rentalRepository.getAllRentals(status)
    }
}
```

**특징**:
- Android Framework 의존성 없음
- 순수 비즈니스 로직만 포함
- 재사용 가능한 Use Case

#### 3. Data Layer (데이터 계층)

**위치**: `data/`

**역할**:
- Repository 인터페이스 구현
- 외부 데이터 소스 관리 (API, DB, 하드웨어)
- 데이터 변환 (DTO ↔ Domain Model)

**구조**:

```
data/
├── api/                # API 인터페이스
│   ├── AuthApi.kt
│   ├── RentalApi.kt
│   └── ...
│
├── model/              # DTO (Data Transfer Object)
│   ├── dto/
│   │   ├── LoginRequestDto.kt
│   │   ├── UserDto.kt
│   │   └── ...
│   └── entity/         # Room Entity
│       ├── VehicleEntity.kt
│       └── ...
│
├── repository/         # Repository 구현
│   ├── AuthRepositoryImpl.kt
│   ├── RentalRepositoryImpl.kt
│   └── fake/           # Fake Repository (Debug)
│       ├── DummyData.kt
│       ├── FakeAuthRepositoryImpl.kt
│       └── ...
│
├── local/              # Room Database
│   ├── AppDatabase.kt
│   ├── dao/
│   │   ├── VehicleDao.kt
│   │   └── ...
│   └── ...
│
└── hardware/           # 하드웨어 매니저
    ├── BodyScanManager.kt
    ├── NFCManager.kt
    ├── BLEManager.kt
    └── ...
```

**Repository 구현 예시**:

```kotlin
// data/repository/RentalRepositoryImpl.kt
@Singleton
class RentalRepositoryImpl @Inject constructor(
    private val rentalApi: RentalApi,
    private val rentalDao: RentalDao,
    private val rentalMapper: RentalMapper
) : RentalRepository {

    override suspend fun getAllRentals(
        status: RentalStatus?,
        page: Int,
        pageSize: Int
    ): Flow<Result<List<Rental>>> = flow {
        emit(Result.Loading)

        try {
            // 1. 로컬 캐시 먼저 확인
            val cachedRentals = rentalDao.getAllRentals()
            if (cachedRentals.isNotEmpty()) {
                emit(Result.Success(
                    cachedRentals.map { rentalMapper.entityToDomain(it) }
                ))
            }

            // 2. API 호출
            val response = rentalApi.getRentals(
                status = status?.name,
                page = page,
                pageSize = pageSize
            )

            // 3. 로컬 DB 업데이트
            val entities = response.data.map { rentalMapper.dtoToEntity(it) }
            rentalDao.insertAll(entities)

            // 4. 최종 결과 emit
            emit(Result.Success(
                response.data.map { rentalMapper.dtoToDomain(it) }
            ))

        } catch (e: Exception) {
            // 캐시가 없으면 에러 emit
            val cachedRentals = rentalDao.getAllRentals()
            if (cachedRentals.isEmpty()) {
                emit(Result.Error(e))
            }
        }
    }
}
```

**Mapper 예시**:

```kotlin
// data/mapper/RentalMapper.kt
@Singleton
class RentalMapper @Inject constructor(
    private val vehicleMapper: VehicleMapper
) {
    // DTO → Domain
    fun dtoToDomain(dto: RentalDto): Rental {
        return Rental(
            rentalId = dto.rentalId,
            userId = dto.userId,
            vehicleId = dto.vehicleId,
            vehicle = vehicleMapper.dtoToDomain(dto.vehicle),
            startTime = dto.startTime,
            endTime = dto.endTime,
            totalPrice = dto.totalPrice,
            status = RentalStatus.valueOf(dto.status),
            pickupLocation = dto.pickupLocation,
            returnLocation = dto.returnLocation,
            createdAt = dto.createdAt,
            updatedAt = dto.updatedAt
        )
    }

    // Domain → Entity
    fun domainToEntity(domain: Rental): RentalEntity {
        return RentalEntity(
            rentalId = domain.rentalId,
            userId = domain.userId,
            vehicleId = domain.vehicleId,
            startTime = domain.startTime,
            endTime = domain.endTime,
            totalPrice = domain.totalPrice,
            status = domain.status.name,
            pickupLocation = domain.pickupLocation,
            returnLocation = domain.returnLocation,
            createdAt = domain.createdAt,
            updatedAt = domain.updatedAt
        )
    }

    // Entity → Domain
    fun entityToDomain(entity: RentalEntity): Rental {
        // ... 변환 로직
    }
}
```

---

## MVVM 패턴

### Model-View-ViewModel 구조

```
┌──────────┐         ┌──────────┐         ┌──────────┐
│   View   │────────►│ViewModel │────────►│  Model   │
│(Compose) │◄────────┤          │◄────────┤(UseCase) │
└──────────┘         └──────────┘         └──────────┘
     │                     │                     │
     │                     │                     │
  UI Events          State/Actions          Business
                                              Logic
```

### ViewModel 구조

```kotlin
@HiltViewModel
class RentalViewModel @Inject constructor(
    private val getRentalsUseCase: GetRentalsUseCase,
    private val createRentalUseCase: CreateRentalUseCase,
    private val cancelRentalUseCase: CancelRentalUseCase
) : ViewModel() {

    // UI State
    private val _uiState = MutableStateFlow<RentalUiState>(RentalUiState.Idle)
    val uiState: StateFlow<RentalUiState> = _uiState.asStateFlow()

    // Events
    private val _events = MutableSharedFlow<RentalEvent>()
    val events: SharedFlow<RentalEvent> = _events.asSharedFlow()

    // Actions
    fun loadRentals(status: RentalStatus? = null) {
        viewModelScope.launch {
            getRentalsUseCase(status).collect { result ->
                _uiState.value = when (result) {
                    is Result.Loading -> RentalUiState.Loading
                    is Result.Success -> RentalUiState.Success(result.data)
                    is Result.Error -> RentalUiState.Error(result.exception.message)
                }
            }
        }
    }

    fun createRental(request: RentalRequest) {
        viewModelScope.launch {
            createRentalUseCase(request).collect { result ->
                when (result) {
                    is Result.Success -> {
                        _events.emit(RentalEvent.RentalCreated(result.data))
                    }
                    is Result.Error -> {
                        _events.emit(RentalEvent.Error(result.exception.message))
                    }
                    else -> {}
                }
            }
        }
    }
}

// UI State
sealed class RentalUiState {
    object Idle : RentalUiState()
    object Loading : RentalUiState()
    data class Success(val rentals: List<Rental>) : RentalUiState()
    data class Error(val message: String?) : RentalUiState()
}

// Events
sealed class RentalEvent {
    data class RentalCreated(val rental: Rental) : RentalEvent()
    data class Error(val message: String?) : RentalEvent()
}
```

---

## 의존성 주입 (Hilt)

### 모듈 구조

#### 1. AppModule

```kotlin
@Module
@InstallIn(SingletonComponent::class)
object AppModule {

    @Provides
    @Singleton
    fun provideContext(@ApplicationContext context: Context): Context {
        return context
    }

    @Provides
    @Singleton
    fun provideDataStore(@ApplicationContext context: Context): DataStore<Preferences> {
        return context.dataStore
    }
}
```

#### 2. NetworkModule

```kotlin
@Module
@InstallIn(SingletonComponent::class)
object NetworkModule {

    private const val BASE_URL = "https://api.mydrive3dx.com/api/"

    @Provides
    @Singleton
    fun provideOkHttpClient(
        authInterceptor: AuthInterceptor
    ): OkHttpClient {
        return OkHttpClient.Builder()
            .addInterceptor(authInterceptor)
            .addInterceptor(HttpLoggingInterceptor().apply {
                level = if (BuildConfig.DEBUG) {
                    HttpLoggingInterceptor.Level.BODY
                } else {
                    HttpLoggingInterceptor.Level.NONE
                }
            })
            .connectTimeout(30, TimeUnit.SECONDS)
            .readTimeout(30, TimeUnit.SECONDS)
            .writeTimeout(30, TimeUnit.SECONDS)
            .build()
    }

    @Provides
    @Singleton
    fun provideRetrofit(okHttpClient: OkHttpClient): Retrofit {
        return Retrofit.Builder()
            .baseUrl(BASE_URL)
            .client(okHttpClient)
            .addConverterFactory(GsonConverterFactory.create())
            .build()
    }

    @Provides
    @Singleton
    fun provideAuthApi(retrofit: Retrofit): AuthApi {
        return retrofit.create(AuthApi::class.java)
    }
}
```

#### 3. RepositoryModule (조건부 주입)

```kotlin
@Module
@InstallIn(SingletonComponent::class)
object RepositoryModule {

    @Provides
    @Singleton
    fun provideAuthRepository(
        tokenManager: TokenManager,
        realImpl: AuthRepositoryImpl,
        fakeImpl: FakeAuthRepositoryImpl
    ): AuthRepository {
        return if (BuildConfig.DEBUG) {
            fakeImpl  // Debug: Fake 데이터 사용
        } else {
            realImpl  // Release: 실제 API 사용
        }
    }

    @Provides
    @Singleton
    fun provideRentalRepository(
        realImpl: RentalRepositoryImpl,
        fakeImpl: FakeRentalRepositoryImpl
    ): RentalRepository {
        return if (BuildConfig.DEBUG) {
            fakeImpl
        } else {
            realImpl
        }
    }

    // ... 다른 Repository들
}
```

#### 4. HardwareModule

```kotlin
@Module
@InstallIn(SingletonComponent::class)
object HardwareModule {

    @Provides
    @Singleton
    fun provideFaceDetector(): FaceDetector {
        val options = FaceDetectorOptions.Builder()
            .setPerformanceMode(FaceDetectorOptions.PERFORMANCE_MODE_ACCURATE)
            .setLandmarkMode(FaceDetectorOptions.LANDMARK_MODE_ALL)
            .setClassificationMode(FaceDetectorOptions.CLASSIFICATION_MODE_ALL)
            .setMinFaceSize(0.15f)
            .enableTracking()
            .build()
        return FaceDetection.getClient(options)
    }

    @Provides
    @Singleton
    fun providePoseDetector(): PoseDetector {
        val options = PoseDetectorOptions.Builder()
            .setDetectorMode(PoseDetectorOptions.STREAM_MODE)
            .build()
        return PoseDetection.getClient(options)
    }

    @Provides
    @Singleton
    fun provideBodyScanManager(
        poseDetector: PoseDetector,
        @ApplicationContext context: Context
    ): BodyScanManager {
        return BodyScanManager(poseDetector, context)
    }
}
```

---

## 데이터 흐름

### 일반적인 데이터 흐름

```
1. 사용자 액션
   ↓
2. UI Event (Composable)
   ↓
3. ViewModel 함수 호출
   ↓
4. Use Case 실행
   ↓
5. Repository 메서드 호출
   ↓
6. API/DB 접근
   ↓
7. 데이터 변환 (DTO → Domain)
   ↓
8. Flow<Result<T>> 반환
   ↓
9. ViewModel에서 수집 (collect)
   ↓
10. UI State 업데이트
    ↓
11. UI 리컴포지션
```

### 실제 예시: 대여 목록 조회

```kotlin
// 1. UI - 사용자가 화면 진입
@Composable
fun RentalListScreen(viewModel: RentalViewModel = hiltViewModel()) {
    LaunchedEffect(Unit) {
        viewModel.loadRentals()  // 2. ViewModel 함수 호출
    }

    val uiState by viewModel.uiState.collectAsState()  // 11. State 수집

    when (uiState) {
        is RentalUiState.Success -> {
            RentalList(rentals = uiState.rentals)  // 12. UI 렌더링
        }
        // ...
    }
}

// 3. ViewModel
@HiltViewModel
class RentalViewModel @Inject constructor(
    private val getRentalsUseCase: GetRentalsUseCase
) : ViewModel() {

    fun loadRentals() {
        viewModelScope.launch {
            getRentalsUseCase()  // 4. Use Case 실행
                .collect { result ->  // 9. 결과 수집
                    _uiState.value = when (result) {  // 10. State 업데이트
                        is Result.Success -> RentalUiState.Success(result.data)
                        is Result.Error -> RentalUiState.Error(result.exception.message)
                        is Result.Loading -> RentalUiState.Loading
                    }
                }
        }
    }
}

// 4-5. Use Case
class GetRentalsUseCase @Inject constructor(
    private val repository: RentalRepository
) {
    suspend operator fun invoke(): Flow<Result<List<Rental>>> {
        return repository.getAllRentals()  // 5. Repository 호출
    }
}

// 6-8. Repository
class RentalRepositoryImpl @Inject constructor(
    private val api: RentalApi,
    private val mapper: RentalMapper
) : RentalRepository {

    override suspend fun getAllRentals(): Flow<Result<List<Rental>>> = flow {
        emit(Result.Loading)

        try {
            val response = api.getRentals()  // 6. API 호출
            val rentals = response.data.map { mapper.dtoToDomain(it) }  // 7. 변환
            emit(Result.Success(rentals))  // 8. Flow emit
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }
}
```

---

## Repository 패턴

### 인터페이스 정의 (Domain)

```kotlin
// domain/repository/AuthRepository.kt
interface AuthRepository {
    suspend fun login(request: LoginRequest): Flow<Result<AuthToken>>
    suspend fun register(request: RegisterRequest): Flow<Result<User>>
    suspend fun logout(): Flow<Result<Unit>>
    suspend fun refreshToken(): Flow<Result<AuthToken>>
    suspend fun getCurrentUser(): Flow<Result<User>>
}
```

### 실제 구현 (Data)

```kotlin
// data/repository/AuthRepositoryImpl.kt
@Singleton
class AuthRepositoryImpl @Inject constructor(
    private val authApi: AuthApi,
    private val tokenManager: TokenManager,
    private val userDao: UserDao,
    private val authMapper: AuthMapper
) : AuthRepository {

    override suspend fun login(request: LoginRequest): Flow<Result<AuthToken>> = flow {
        emit(Result.Loading)

        try {
            val response = authApi.login(authMapper.loginRequestToDto(request))

            if (response.success) {
                val token = authMapper.tokenDtoToDomain(response.data)
                tokenManager.saveToken(token)
                emit(Result.Success(token))
            } else {
                emit(Result.Error(Exception(response.message)))
            }
        } catch (e: Exception) {
            emit(Result.Error(e))
        }
    }
}
```

### Fake 구현 (Debug)

```kotlin
// data/repository/fake/FakeAuthRepositoryImpl.kt
@Singleton
class FakeAuthRepositoryImpl @Inject constructor(
    private val tokenManager: TokenManager
) : AuthRepository {

    override suspend fun login(request: LoginRequest): Flow<Result<AuthToken>> = flow {
        emit(Result.Loading)
        delay(500)  // 네트워크 시뮬레이션

        if (request.email.isNotEmpty() && request.password.isNotEmpty()) {
            tokenManager.saveToken(DummyData.dummyAuthToken)
            emit(Result.Success(DummyData.dummyAuthToken))
        } else {
            emit(Result.Error(Exception("이메일과 비밀번호를 입력해주세요")))
        }
    }
}
```

---

## 상태 관리

### Result 패턴

```kotlin
sealed class Result<out T> {
    data class Success<T>(val data: T) : Result<T>()
    data class Error(val exception: Exception) : Result<Nothing>()
    object Loading : Result<Nothing>()
}
```

### StateFlow vs SharedFlow

```kotlin
class ExampleViewModel : ViewModel() {

    // StateFlow - 상태 관리 (항상 최신 값 보유)
    private val _uiState = MutableStateFlow<UiState>(UiState.Idle)
    val uiState: StateFlow<UiState> = _uiState.asStateFlow()

    // SharedFlow - 이벤트 관리 (일회성)
    private val _events = MutableSharedFlow<Event>()
    val events: SharedFlow<Event> = _events.asSharedFlow()

    fun loadData() {
        viewModelScope.launch {
            _uiState.value = UiState.Loading

            try {
                val data = repository.getData()
                _uiState.value = UiState.Success(data)
                _events.emit(Event.DataLoaded)  // 일회성 이벤트
            } catch (e: Exception) {
                _uiState.value = UiState.Error(e.message)
                _events.emit(Event.ShowError(e.message))
            }
        }
    }
}
```

---

## 에러 처리

### 계층별 에러 처리

```kotlin
// 1. Repository - 에러 캐치 및 Result로 래핑
override suspend fun getData(): Flow<Result<Data>> = flow {
    emit(Result.Loading)

    try {
        val response = api.getData()
        emit(Result.Success(response.toDomain()))
    } catch (e: HttpException) {
        emit(Result.Error(Exception("네트워크 오류: ${e.code()}")))
    } catch (e: IOException) {
        emit(Result.Error(Exception("연결 실패")))
    } catch (e: Exception) {
        emit(Result.Error(Exception("알 수 없는 오류")))
    }
}

// 2. ViewModel - UI State로 변환
fun loadData() {
    viewModelScope.launch {
        repository.getData().collect { result ->
            _uiState.value = when (result) {
                is Result.Loading -> UiState.Loading
                is Result.Success -> UiState.Success(result.data)
                is Result.Error -> UiState.Error(
                    message = result.exception.message ?: "오류 발생"
                )
            }
        }
    }
}

// 3. UI - 사용자에게 표시
when (uiState) {
    is UiState.Error -> {
        ErrorDialog(
            message = uiState.message,
            onDismiss = { viewModel.clearError() }
        )
    }
    // ...
}
```

---

## 테스트 전략

### 1. Unit Tests

```kotlin
// ViewModel Test
@Test
fun `login with valid credentials returns success`() = runTest {
    // Given
    val loginRequest = LoginRequest("test@test.com", "password")
    val expectedToken = AuthToken("token", "refresh", 3600)

    coEvery { loginUseCase(loginRequest) } returns flow {
        emit(Result.Success(expectedToken))
    }

    // When
    viewModel.login(loginRequest.email, loginRequest.password)

    // Then
    val state = viewModel.uiState.value
    assertTrue(state is AuthUiState.Success)
    assertEquals(expectedToken, (state as AuthUiState.Success).token)
}
```

### 2. Repository Tests

```kotlin
@Test
fun `getRentals returns cached data when API fails`() = runTest {
    // Given
    val cachedRentals = listOf(/* cached data */)
    coEvery { rentalDao.getAllRentals() } returns cachedRentals
    coEvery { rentalApi.getRentals() } throws IOException()

    // When
    val result = repository.getAllRentals().first()

    // Then
    assertTrue(result is Result.Success)
    assertEquals(cachedRentals.size, (result as Result.Success).data.size)
}
```

### 3. UI Tests (Compose)

```kotlin
@Test
fun loginScreen_displaysErrorOnInvalidCredentials() {
    composeTestRule.setContent {
        LoginScreen(
            viewModel = fakeViewModel,
            onLoginSuccess = {}
        )
    }

    // When
    composeTestRule.onNodeWithTag("email_field").performTextInput("invalid")
    composeTestRule.onNodeWithTag("password_field").performTextInput("123")
    composeTestRule.onNodeWithTag("login_button").performClick()

    // Then
    composeTestRule.onNodeWithText("이메일 형식이 올바르지 않습니다").assertIsDisplayed()
}
```

---

## 요약

### 핵심 아키텍처 원칙

1. **Clean Architecture**로 계층 분리
2. **MVVM 패턴**으로 UI와 비즈니스 로직 분리
3. **Hilt**를 통한 의존성 주입
4. **Repository 패턴**으로 데이터 소스 추상화
5. **Flow**를 통한 반응형 데이터 스트림
6. **Result 패턴**으로 통일된 에러 처리
7. **Debug/Release 빌드**로 Fake/Real 데이터 분리

### 아키텍처 장점

✅ **유지보수성**: 계층별 독립적 수정 가능
✅ **테스트 용이성**: Mock/Fake 객체로 쉬운 테스트
✅ **확장성**: 새 기능 추가 시 기존 코드 영향 최소화
✅ **재사용성**: Use Case 및 Repository 재사용
✅ **가독성**: 명확한 책임 분리로 코드 이해 쉬움

---

**Last Updated**: 2025-10-03
