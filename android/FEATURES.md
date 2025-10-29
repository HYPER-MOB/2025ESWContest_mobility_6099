# MyDrive3DX - Features Documentation

## 목차

1. [인증 시스템 (Authentication)](#인증-시스템-authentication)
2. [Multi-Factor Authentication (MFA)](#multi-factor-authentication-mfa)
3. [차량 대여 시스템](#차량-대여-시스템)
4. [원격 차량 제어](#원격-차량-제어)
5. [3D 바디스캔](#3d-바디스캔)
6. [자동 차량 세팅](#자동-차량-세팅)
7. [졸음 감지 시스템](#졸음-감지-시스템)
8. [프로필 관리](#프로필-관리)

---

## 인증 시스템 (Authentication)

### 개요

JWT 기반 토큰 인증 시스템으로 안전한 사용자 인증을 제공합니다.

### 주요 기능

#### 1. 로그인

**화면**: `LoginScreen.kt`

```kotlin
@Composable
fun LoginScreen(
    viewModel: AuthViewModel = hiltViewModel(),
    onLoginSuccess: () -> Unit,
    onNavigateToRegister: () -> Unit
) {
    var email by remember { mutableStateOf("") }
    var password by remember { mutableStateOf("") }

    Column {
        OutlinedTextField(
            value = email,
            onValueChange = { email = it },
            label = { Text("이메일") },
            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Email)
        )

        OutlinedTextField(
            value = password,
            onValueChange = { password = it },
            label = { Text("비밀번호") },
            visualTransformation = PasswordVisualTransformation()
        )

        Button(onClick = { viewModel.login(email, password) }) {
            Text("로그인")
        }
    }
}
```

**처리 흐름**:
1. 사용자가 이메일/비밀번호 입력
2. `AuthViewModel.login()` 호출
3. `LoginUseCase` 실행
4. `AuthRepository`를 통해 API 호출
5. 성공 시 토큰 저장 및 메인 화면 이동

#### 2. 회원가입

**필수 정보**:
- 이메일
- 비밀번호
- 이름
- 전화번호
- 운전면허번호

**검증 로직**:
```kotlin
fun validateRegisterInput(request: RegisterRequest): ValidationResult {
    return when {
        !request.email.isValidEmail() ->
            ValidationResult.Error("이메일 형식이 올바르지 않습니다")

        request.password.length < 8 ->
            ValidationResult.Error("비밀번호는 8자 이상이어야 합니다")

        !request.phone.isValidPhone() ->
            ValidationResult.Error("전화번호 형식이 올바르지 않습니다")

        !request.drivingLicense.isValidLicense() ->
            ValidationResult.Error("운전면허번호 형식이 올바르지 않습니다")

        else -> ValidationResult.Success
    }
}
```

#### 3. 토큰 관리

**TokenManager**:
```kotlin
@Singleton
class TokenManager @Inject constructor(
    private val dataStore: DataStore<Preferences>
) {
    private val ACCESS_TOKEN_KEY = stringPreferencesKey("access_token")
    private val REFRESH_TOKEN_KEY = stringPreferencesKey("refresh_token")

    suspend fun saveToken(token: AuthToken) {
        dataStore.edit { preferences ->
            preferences[ACCESS_TOKEN_KEY] = token.accessToken
            preferences[REFRESH_TOKEN_KEY] = token.refreshToken
        }
    }

    suspend fun getAccessToken(): String? {
        return dataStore.data.map { it[ACCESS_TOKEN_KEY] }.first()
    }

    suspend fun clearTokens() {
        dataStore.edit { it.clear() }
    }
}
```

**자동 토큰 갱신**:
```kotlin
class AuthInterceptor @Inject constructor(
    private val tokenManager: TokenManager,
    private val authApi: AuthApi
) : Interceptor {

    override fun intercept(chain: Chain): Response {
        val request = chain.request()
        val token = runBlocking { tokenManager.getAccessToken() }

        val authenticatedRequest = request.newBuilder()
            .addHeader("Authorization", "Bearer $token")
            .build()

        val response = chain.proceed(authenticatedRequest)

        // 토큰 만료 시 자동 갱신
        if (response.code == 401) {
            response.close()

            val newToken = runBlocking {
                val refreshToken = tokenManager.getRefreshToken()
                authApi.refreshToken(RefreshTokenRequest(refreshToken))
            }

            runBlocking { tokenManager.saveToken(newToken) }

            val newRequest = request.newBuilder()
                .addHeader("Authorization", "Bearer ${newToken.accessToken}")
                .build()

            return chain.proceed(newRequest)
        }

        return response
    }
}
```

---

## Multi-Factor Authentication (MFA)

### 개요

차량 인수 시 3단계 인증을 통해 보안을 강화합니다.

### 인증 단계

#### 1단계: 얼굴 인식 (Face Recognition)

**기술**: ML Kit Face Detection

```kotlin
@Singleton
class FaceDetectionManager @Inject constructor(
    private val faceDetector: FaceDetector,
    @ApplicationContext private val context: Context
) {
    suspend fun verifyFace(imageProxy: ImageProxy): FaceVerificationResult {
        val mediaImage = imageProxy.image ?: return FaceVerificationResult.Error

        val inputImage = InputImage.fromMediaImage(
            mediaImage,
            imageProxy.imageInfo.rotationDegrees
        )

        return suspendCoroutine { continuation ->
            faceDetector.process(inputImage)
                .addOnSuccessListener { faces ->
                    if (faces.isEmpty()) {
                        continuation.resume(FaceVerificationResult.NoFaceDetected)
                    } else {
                        val face = faces.first()

                        // 얼굴 각도 확인
                        val headRotationY = face.headEulerAngleY
                        val headRotationZ = face.headEulerAngleZ

                        if (abs(headRotationY) > 20 || abs(headRotationZ) > 20) {
                            continuation.resume(FaceVerificationResult.InvalidPose)
                        } else {
                            continuation.resume(FaceVerificationResult.Success(face))
                        }
                    }
                }
                .addOnFailureListener { e ->
                    continuation.resume(FaceVerificationResult.Error)
                }
        }
    }
}
```

**검증 조건**:
- 얼굴이 정면을 향할 것
- 머리 회전 각도 ±20도 이내
- 눈이 떠져 있을 것 (눈 감김 확률 < 0.3)

#### 2단계: NFC 인증

**기술**: Android NFC API

```kotlin
@Singleton
class NFCManager @Inject constructor(
    @ApplicationContext private val context: Context
) {
    private val nfcAdapter: NfcAdapter? = NfcAdapter.getDefaultAdapter(context)

    fun isNfcEnabled(): Boolean {
        return nfcAdapter?.isEnabled == true
    }

    suspend fun verifyNfcTag(intent: Intent): NFCVerificationResult {
        val tag = intent.getParcelableExtra<Tag>(NfcAdapter.EXTRA_TAG)
            ?: return NFCVerificationResult.NoTagDetected

        val tagId = tag.id.toHexString()

        // 서버에 태그 ID 검증 요청
        return try {
            val response = api.verifyNfcTag(tagId)
            if (response.isValid) {
                NFCVerificationResult.Success(tagId)
            } else {
                NFCVerificationResult.InvalidTag
            }
        } catch (e: Exception) {
            NFCVerificationResult.Error(e.message)
        }
    }
}
```

**처리 흐름**:
1. NFC 태그 감지
2. 태그 ID 추출
3. 서버에 검증 요청
4. 차량 정보와 매칭 확인

#### 3단계: BLE 인증

**기술**: Bluetooth Low Energy

```kotlin
@Singleton
class BLEManager @Inject constructor(
    @ApplicationContext private val context: Context
) {
    private val bluetoothAdapter: BluetoothAdapter? =
        BluetoothAdapter.getDefaultAdapter()

    suspend fun scanForVehicle(vehicleId: String): BLEVerificationResult {
        if (!isBluetoothEnabled()) {
            return BLEVerificationResult.BluetoothDisabled
        }

        return withTimeoutOrNull(10000) {
            // BLE 스캔 시작
            val scanner = bluetoothAdapter?.bluetoothLeScanner

            scanner?.startScan(object : ScanCallback() {
                override fun onScanResult(callbackType: Int, result: ScanResult) {
                    val deviceName = result.device.name

                    // 차량 이름과 매칭
                    if (deviceName?.contains(vehicleId) == true) {
                        // 연결 시도
                        result.device.connectGatt(context, false, gattCallback)
                    }
                }
            })

            // 결과 대기...
        } ?: BLEVerificationResult.Timeout
    }
}
```

**검증 조건**:
- Bluetooth 활성화 확인
- 차량 BLE 신호 감지
- GATT 연결 성공
- 인증 특성(Characteristic) 읽기 성공

### MFA 통합 화면

**MfaScreen.kt**:
```kotlin
@Composable
fun MfaScreen(
    rentalId: String,
    viewModel: MfaViewModel = hiltViewModel()
) {
    val mfaState by viewModel.mfaState.collectAsState()

    Column {
        // 1. 얼굴 인식
        MfaStep(
            stepNumber = 1,
            title = "얼굴 인식",
            isCompleted = mfaState.faceVerified,
            isActive = mfaState.currentStep == MfaStep.FACE
        ) {
            if (mfaState.currentStep == MfaStep.FACE) {
                FaceVerificationView(onVerified = viewModel::onFaceVerified)
            }
        }

        // 2. NFC 인증
        MfaStep(
            stepNumber = 2,
            title = "NFC 카드 태그",
            isCompleted = mfaState.nfcVerified,
            isActive = mfaState.currentStep == MfaStep.NFC
        ) {
            if (mfaState.currentStep == MfaStep.NFC) {
                NFCVerificationView(onVerified = viewModel::onNfcVerified)
            }
        }

        // 3. BLE 인증
        MfaStep(
            stepNumber = 3,
            title = "차량 Bluetooth 연결",
            isCompleted = mfaState.bleVerified,
            isActive = mfaState.currentStep == MfaStep.BLE
        ) {
            if (mfaState.currentStep == MfaStep.BLE) {
                BLEVerificationView(onVerified = viewModel::onBleVerified)
            }
        }
    }

    // 모든 인증 완료 시
    LaunchedEffect(mfaState.allVerified) {
        if (mfaState.allVerified) {
            viewModel.completePickup(rentalId)
        }
    }
}
```

---

## 차량 대여 시스템

### 개요

차량 검색부터 예약, 인수, 반납까지 전체 대여 프로세스를 관리합니다.

### 주요 기능

#### 1. 차량 검색 및 필터링

**VehicleListScreen.kt**:
```kotlin
@Composable
fun VehicleListScreen(viewModel: VehicleViewModel = hiltViewModel()) {
    var selectedType by remember { mutableStateOf<VehicleType?>(null) }
    var selectedFuelType by remember { mutableStateOf<FuelType?>(null) }

    LaunchedEffect(selectedType, selectedFuelType) {
        viewModel.filterVehicles(type = selectedType, fuelType = selectedFuelType)
    }

    Column {
        // 필터 칩
        FilterChips(
            selectedType = selectedType,
            onTypeSelected = { selectedType = it }
        )

        // 차량 목록
        LazyColumn {
            items(vehicles) { vehicle ->
                VehicleCard(
                    vehicle = vehicle,
                    onClick = { navigateToDetail(vehicle.vehicleId) }
                )
            }
        }
    }
}
```

**필터 옵션**:
- 차량 유형 (SEDAN, SUV, TRUCK, VAN, etc.)
- 연료 타입 (GASOLINE, DIESEL, ELECTRIC, HYBRID)
- 가격 범위
- 위치
- 좌석 수

#### 2. 차량 상세 정보

**VehicleDetailScreen.kt**:
```kotlin
@Composable
fun VehicleDetailScreen(
    vehicleId: String,
    viewModel: VehicleViewModel = hiltViewModel()
) {
    val vehicle by viewModel.selectedVehicle.collectAsState()

    vehicle?.let { v ->
        Column {
            // 차량 이미지
            AsyncImage(
                model = v.imageUrl,
                contentDescription = v.model
            )

            // 기본 정보
            Text("${v.manufacturer} ${v.model}", style = MaterialTheme.typography.headlineMedium)
            Text("${v.year}년 · ${v.color}")

            // 스펙
            Row {
                SpecItem(icon = Icons.Default.Person, text = "${v.seatingCapacity}인승")
                SpecItem(icon = Icons.Default.Settings, text = v.transmission.displayName)
                SpecItem(icon = Icons.Default.LocalGasStation, text = v.fuelType.displayName)
            }

            // 가격
            PriceSection(
                pricePerHour = v.pricePerHour,
                pricePerDay = v.pricePerDay
            )

            // 위치
            LocationSection(
                location = v.location,
                latitude = v.latitude,
                longitude = v.longitude
            )

            // 특징
            FeatureChips(features = v.features)

            // 예약 버튼
            Button(
                onClick = { navigateToRentalRequest(vehicleId) },
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("예약하기")
            }
        }
    }
}
```

#### 3. 대여 요청

**RentalRequestScreen.kt**:
```kotlin
@Composable
fun RentalRequestScreen(
    vehicleId: String,
    viewModel: RentalViewModel = hiltViewModel()
) {
    var startDate by remember { mutableStateOf<LocalDateTime?>(null) }
    var endDate by remember { mutableStateOf<LocalDateTime?>(null) }

    Column {
        // 날짜 선택
        DateRangePicker(
            startDate = startDate,
            endDate = endDate,
            onStartDateSelected = { startDate = it },
            onEndDateSelected = { endDate = it }
        )

        // 가격 계산
        PriceCalculation(
            vehicle = vehicle,
            startDate = startDate,
            endDate = endDate
        )

        // 약관 동의
        AgreementCheckboxes()

        // 예약 버튼
        Button(
            onClick = {
                viewModel.createRental(
                    RentalRequest(
                        vehicleId = vehicleId,
                        startTime = startDate.toString(),
                        endTime = endDate.toString()
                    )
                )
            },
            enabled = startDate != null && endDate != null
        ) {
            Text("예약 확정")
        }
    }
}
```

#### 4. 대여 관리

**대여 상태 (RentalStatus)**:
```kotlin
enum class RentalStatus {
    REQUESTED,  // 대여 요청됨
    APPROVED,   // 대여 승인됨
    PICKED,     // 차량 인수함
    RETURNED,   // 차량 반납함
    CANCELED    // 대여 취소됨
}
```

**상태별 액션**:
- REQUESTED → 취소 가능
- APPROVED → MFA 인증 후 인수
- PICKED → 운행 중, 반납 가능
- RETURNED → 완료
- CANCELED → 종료

---

## 원격 차량 제어

### 개요

앱에서 차량을 원격으로 제어할 수 있는 기능을 제공합니다.

### 제어 항목

#### 1. 도어 제어

```kotlin
@Composable
fun DoorControlSection(viewModel: VehicleControlViewModel) {
    val doorStatus by viewModel.doorStatus.collectAsState()

    Row {
        // 전체 잠금/해제
        Button(onClick = {
            if (doorStatus.isAllLocked) {
                viewModel.unlockDoors()
            } else {
                viewModel.lockDoors()
            }
        }) {
            Icon(
                imageVector = if (doorStatus.isAllLocked) Icons.Default.Lock else Icons.Default.LockOpen,
                contentDescription = null
            )
            Text(if (doorStatus.isAllLocked) "문 열기" else "문 잠금")
        }
    }

    // 개별 도어 상태
    DoorStatusGrid(
        frontLeft = doorStatus.frontLeft,
        frontRight = doorStatus.frontRight,
        rearLeft = doorStatus.rearLeft,
        rearRight = doorStatus.rearRight,
        trunk = doorStatus.trunk
    )
}
```

#### 2. 시동 제어

```kotlin
@Composable
fun EngineControlSection(viewModel: VehicleControlViewModel) {
    val engineStatus by viewModel.engineStatus.collectAsState()

    Card {
        Column {
            Text("엔진 상태")

            Row {
                // 상태 표시
                StatusIndicator(
                    isActive = engineStatus.isRunning,
                    activeText = "시동 켜짐",
                    inactiveText = "시동 꺼짐"
                )

                // RPM 표시
                if (engineStatus.isRunning) {
                    Text("${engineStatus.rpm} RPM")
                }
            }

            // 제어 버튼
            Button(
                onClick = {
                    if (engineStatus.isRunning) {
                        viewModel.stopEngine()
                    } else {
                        viewModel.startEngine()
                    }
                }
            ) {
                Text(if (engineStatus.isRunning) "시동 끄기" else "시동 켜기")
            }
        }
    }
}
```

#### 3. 에어컨 제어

```kotlin
@Composable
fun ClimateControlSection(viewModel: VehicleControlViewModel) {
    val climateStatus by viewModel.climateStatus.collectAsState()
    var targetTemperature by remember { mutableStateOf(22.0) }

    Card {
        Column {
            Text("에어컨")

            // 온/오프 스위치
            Switch(
                checked = climateStatus.isOn,
                onCheckedChange = {
                    if (it) {
                        viewModel.turnOnClimate(targetTemperature)
                    } else {
                        viewModel.turnOffClimate()
                    }
                }
            )

            if (climateStatus.isOn) {
                // 온도 조절
                Row {
                    Text("온도: ${climateStatus.temperature}°C")

                    Slider(
                        value = targetTemperature.toFloat(),
                        onValueChange = { targetTemperature = it.toDouble() },
                        valueRange = 16f..30f,
                        onValueChangeFinished = {
                            viewModel.setTemperature(targetTemperature)
                        }
                    )
                }

                // 풍량 조절
                Text("풍량: ${climateStatus.fanSpeed}")
            }
        }
    }
}
```

#### 4. 위치 확인

```kotlin
@Composable
fun VehicleLocationSection(viewModel: VehicleControlViewModel) {
    val location by viewModel.vehicleLocation.collectAsState()

    Card {
        Column {
            Text("차량 위치")

            // 지도 표시 (Google Maps Compose)
            GoogleMap(
                modifier = Modifier.height(200.dp),
                cameraPositionState = rememberCameraPositionState {
                    position = CameraPosition.fromLatLngZoom(
                        LatLng(location.latitude, location.longitude),
                        15f
                    )
                }
            ) {
                Marker(
                    state = MarkerState(
                        position = LatLng(location.latitude, location.longitude)
                    ),
                    title = "내 차량"
                )
            }

            Text(location.address)

            // 경적/라이트 버튼
            Row {
                Button(onClick = { viewModel.honkHorn() }) {
                    Icon(Icons.Default.Notifications, null)
                    Text("경적")
                }

                Button(onClick = { viewModel.flashLights() }) {
                    Icon(Icons.Default.Lightbulb, null)
                    Text("라이트")
                }
            }
        }
    }
}
```

---

## 3D 바디스캔

### 개요

ML Kit Pose Detection을 활용하여 사용자의 신체 치수를 측정합니다.

### 측정 항목

**7가지 신체 치수**:
1. Sitting Height (앉은키)
2. Shoulder Width (어깨 너비)
3. Arm Length (팔 길이)
4. Head Height (머리 높이)
5. Eye Height (눈 높이)
6. Leg Length (다리 길이)
7. Torso Length (상체 길이)

### 구현

#### BodyScanManager

```kotlin
@Singleton
class BodyScanManager @Inject constructor(
    private val poseDetector: PoseDetector,
    @ApplicationContext private val context: Context
) {
    suspend fun scanBody(imageProxy: ImageProxy): BodyScanResult {
        val mediaImage = imageProxy.image ?: return BodyScanResult.NoImageError

        val inputImage = InputImage.fromMediaImage(
            mediaImage,
            imageProxy.imageInfo.rotationDegrees
        )

        return suspendCoroutine { continuation ->
            poseDetector.process(inputImage)
                .addOnSuccessListener { pose ->
                    if (pose.allPoseLandmarks.isEmpty()) {
                        continuation.resume(BodyScanResult.NoPoseDetected)
                    } else {
                        val measurements = calculateMeasurements(pose)
                        continuation.resume(BodyScanResult.Success(measurements))
                    }
                }
                .addOnFailureListener { e ->
                    continuation.resume(BodyScanResult.Error(e.message))
                }
        }
    }

    private fun calculateMeasurements(pose: Pose): BodyMeasurements {
        // 랜드마크 추출
        val nose = pose.getPoseLandmark(PoseLandmark.NOSE)
        val leftShoulder = pose.getPoseLandmark(PoseLandmark.LEFT_SHOULDER)
        val rightShoulder = pose.getPoseLandmark(PoseLandmark.RIGHT_SHOULDER)
        val leftHip = pose.getPoseLandmark(PoseLandmark.LEFT_HIP)
        val rightHip = pose.getPoseLandmark(PoseLandmark.RIGHT_HIP)
        val leftKnee = pose.getPoseLandmark(PoseLandmark.LEFT_KNEE)
        val rightKnee = pose.getPoseLandmark(PoseLandmark.RIGHT_KNEE)
        val leftAnkle = pose.getPoseLandmark(PoseLandmark.LEFT_ANKLE)
        val rightAnkle = pose.getPoseLandmark(PoseLandmark.RIGHT_ANKLE)
        val leftWrist = pose.getPoseLandmark(PoseLandmark.LEFT_WRIST)
        val rightWrist = pose.getPoseLandmark(PoseLandmark.RIGHT_WRIST)
        val leftElbow = pose.getPoseLandmark(PoseLandmark.LEFT_ELBOW)
        val rightElbow = pose.getPoseLandmark(PoseLandmark.RIGHT_ELBOW)

        return BodyMeasurements(
            // 어깨 너비
            shoulderWidth = calculateDistance(leftShoulder, rightShoulder),

            // 팔 길이 (어깨-팔꿈치-손목)
            armLength = (
                calculateDistance(leftShoulder, leftElbow) +
                calculateDistance(leftElbow, leftWrist)
            ) / 2,

            // 머리 높이 (코-어깨 중심)
            headHeight = calculateDistance(
                nose,
                midpoint(leftShoulder, rightShoulder)
            ),

            // 눈 높이 (앉은키 + 머리 높이)
            eyeHeight = calculateSittingHeight(pose) + calculateHeadHeight(pose),

            // 다리 길이 (엉덩이-무릎-발목)
            legLength = (
                calculateDistance(leftHip, leftKnee) +
                calculateDistance(leftKnee, leftAnkle)
            ),

            // 상체 길이 (어깨-엉덩이)
            torsoLength = calculateDistance(
                midpoint(leftShoulder, rightShoulder),
                midpoint(leftHip, rightHip)
            ),

            // 앉은키 (엉덩이-어깨-머리)
            sittingHeight = calculateSittingHeight(pose)
        )
    }

    private fun calculateDistance(p1: PoseLandmark?, p2: PoseLandmark?): Double {
        if (p1 == null || p2 == null) return 0.0

        val dx = p1.position.x - p2.position.x
        val dy = p1.position.y - p2.position.y

        return sqrt(dx * dx + dy * dy).toDouble()
    }
}
```

#### BodyScanScreen

```kotlin
@Composable
fun BodyScanScreen(
    viewModel: BodyScanViewModel = hiltViewModel(),
    onScanComplete: (BodyScan) -> Unit
) {
    val scanState by viewModel.scanState.collectAsState()

    Column {
        // 카메라 프리뷰
        CameraPreview(
            onImageCaptured = { imageProxy ->
                viewModel.processScan(imageProxy)
            }
        )

        // 가이드 오버레이
        ScanGuideOverlay(
            guideText = when (scanState.currentPose) {
                ScanPose.FRONT -> "정면을 향해 서주세요"
                ScanPose.SIDE -> "옆으로 서주세요"
                ScanPose.SITTING -> "의자에 앉아주세요"
            }
        )

        // 진행 상황
        LinearProgressIndicator(
            progress = scanState.progress,
            modifier = Modifier.fillMaxWidth()
        )

        // 측정 결과 (실시간)
        if (scanState.measurements != null) {
            MeasurementResults(measurements = scanState.measurements)
        }
    }

    // 스캔 완료
    LaunchedEffect(scanState.isComplete) {
        if (scanState.isComplete) {
            val bodyScan = BodyScan(
                scanId = UUID.randomUUID().toString(),
                userId = viewModel.currentUserId,
                timestamp = LocalDateTime.now().toString(),
                sittingHeight = scanState.measurements.sittingHeight,
                shoulderWidth = scanState.measurements.shoulderWidth,
                armLength = scanState.measurements.armLength,
                headHeight = scanState.measurements.headHeight,
                eyeHeight = scanState.measurements.eyeHeight,
                legLength = scanState.measurements.legLength,
                torsoLength = scanState.measurements.torsoLength,
                weight = scanState.weight,
                height = scanState.height
            )

            onScanComplete(bodyScan)
        }
    }
}
```

---

## 자동 차량 세팅

### 개요

바디스캔 데이터를 기반으로 차량 세팅을 자동으로 계산하고 적용합니다.

### 세팅 항목

**8축 자동 조정**:
1. Seat Front/Back (시트 전후)
2. Seat Up/Down (시트 상하)
3. Seat Back Angle (등받이 각도)
4. Headrest Height (머리 받침 높이)
5. Left Mirror Angle (좌측 미러)
6. Right Mirror Angle (우측 미러)
7. Steering Wheel Height (핸들 높이)
8. Steering Wheel Depth (핸들 깊이)

### 계산 알고리즘

```kotlin
object VehicleSettingsCalculator {

    fun calculateSettings(
        bodyScan: BodyScan,
        vehicleModel: String
    ): VehicleSettings {
        // 차량별 기준값
        val baseSettings = getBaseSettings(vehicleModel)

        return VehicleSettings(
            // 1. 시트 전후 위치
            seatFrontBack = calculateSeatPosition(
                legLength = bodyScan.legLength,
                height = bodyScan.height,
                basePosition = baseSettings.seatFrontBack
            ),

            // 2. 시트 높이
            seatUpDown = calculateSeatHeight(
                sittingHeight = bodyScan.sittingHeight,
                eyeHeight = bodyScan.eyeHeight,
                baseHeight = baseSettings.seatUpDown
            ),

            // 3. 등받이 각도
            seatBackAngle = calculateBackAngle(
                torsoLength = bodyScan.torsoLength,
                weight = bodyScan.weight,
                baseAngle = baseSettings.seatBackAngle
            ),

            // 4. 머리 받침
            seatHeadrestHeight = calculateHeadrestHeight(
                headHeight = bodyScan.headHeight,
                sittingHeight = bodyScan.sittingHeight,
                baseHeight = baseSettings.seatHeadrestHeight
            ),

            // 5-6. 사이드 미러
            leftMirrorAngle = calculateMirrorAngle(
                eyeHeight = bodyScan.eyeHeight,
                shoulderWidth = bodyScan.shoulderWidth,
                side = MirrorSide.LEFT
            ),
            rightMirrorAngle = calculateMirrorAngle(
                eyeHeight = bodyScan.eyeHeight,
                shoulderWidth = bodyScan.shoulderWidth,
                side = MirrorSide.RIGHT
            ),

            // 7-8. 핸들 위치
            steeringWheelHeight = calculateSteeringHeight(
                sittingHeight = bodyScan.sittingHeight,
                armLength = bodyScan.armLength
            ),
            steeringWheelDepth = calculateSteeringDepth(
                armLength = bodyScan.armLength,
                torsoLength = bodyScan.torsoLength
            ),

            vehicleModel = vehicleModel,
            calibrationVersion = "1.0"
        )
    }

    private fun calculateSeatPosition(
        legLength: Double,
        height: Double,
        basePosition: Double
    ): Double {
        // 다리 길이에 따른 시트 위치 조정
        val legRatio = legLength / height
        val adjustment = (legRatio - 0.45) * 100  // 기준 비율 0.45

        return (basePosition + adjustment).coerceIn(0.0, 100.0)
    }

    private fun calculateSeatHeight(
        sittingHeight: Double,
        eyeHeight: Double,
        baseHeight: Double
    ): Double {
        // 앉은키와 눈 높이를 고려한 시트 높이
        val heightRatio = sittingHeight / eyeHeight
        val adjustment = (0.6 - heightRatio) * 100  // 기준 비율 0.6

        return (baseHeight + adjustment).coerceIn(0.0, 100.0)
    }

    // ... 다른 계산 함수들
}
```

### 세팅 적용

```kotlin
@Composable
fun ApplySettingsScreen(
    profileId: String,
    vehicleId: String,
    viewModel: BodyScanViewModel = hiltViewModel()
) {
    val applyState by viewModel.applyState.collectAsState()

    Column {
        when (applyState) {
            is ApplyState.Idle -> {
                Text("세팅을 적용하시겠습니까?")

                Button(onClick = {
                    viewModel.applySettings(profileId, vehicleId)
                }) {
                    Text("세팅 적용")
                }
            }

            is ApplyState.Applying -> {
                Column {
                    Text("세팅 적용 중...")

                    LinearProgressIndicator(
                        progress = applyState.progress
                    )

                    Text("예상 시간: ${applyState.estimatedTime}초")

                    // 실시간 적용 상태
                    SettingProgressList(
                        items = listOf(
                            SettingItem("시트 전후", applyState.seatFrontBackDone),
                            SettingItem("시트 상하", applyState.seatUpDownDone),
                            SettingItem("등받이 각도", applyState.backAngleDone),
                            SettingItem("머리 받침", applyState.headrestDone),
                            SettingItem("좌측 미러", applyState.leftMirrorDone),
                            SettingItem("우측 미러", applyState.rightMirrorDone),
                            SettingItem("핸들 높이", applyState.steeringHeightDone),
                            SettingItem("핸들 깊이", applyState.steeringDepthDone)
                        )
                    )
                }
            }

            is ApplyState.Success -> {
                Column {
                    Icon(
                        imageVector = Icons.Default.CheckCircle,
                        contentDescription = null,
                        tint = Color.Green,
                        modifier = Modifier.size(64.dp)
                    )

                    Text("세팅이 완료되었습니다!")

                    Button(onClick = onComplete) {
                        Text("확인")
                    }
                }
            }

            is ApplyState.Error -> {
                ErrorMessage(
                    message = applyState.message,
                    onRetry = { viewModel.applySettings(profileId, vehicleId) }
                )
            }
        }
    }
}
```

---

## 졸음 감지 시스템

### 개요

ML Kit Face Detection을 활용하여 운전 중 졸음을 실시간으로 감지합니다.

### 감지 항목

1. **눈 감김 (Eye Closure)**
   - 좌/우 눈 감김 확률 측정
   - 연속 감김 시간 측정

2. **하품 (Yawning)**
   - 입 벌림 정도 측정
   - 하품 패턴 인식

3. **고개 숙임 (Head Nodding)**
   - 머리 각도 변화 추적
   - 고개 숙임 빈도 측정

### 구현

```kotlin
@Singleton
class DrowsinessDetectionManager @Inject constructor(
    private val faceDetector: FaceDetector,
    @ApplicationContext private val context: Context
) {
    private var eyeClosedStartTime: Long? = null
    private var eyeClosedDuration = 0L
    private var yawnCount = 0
    private var headNodCount = 0

    suspend fun detectDrowsiness(imageProxy: ImageProxy): DrowsinessResult {
        val mediaImage = imageProxy.image ?: return DrowsinessResult.NoImageError

        val inputImage = InputImage.fromMediaImage(
            mediaImage,
            imageProxy.imageInfo.rotationDegrees
        )

        return suspendCoroutine { continuation ->
            faceDetector.process(inputImage)
                .addOnSuccessListener { faces ->
                    if (faces.isEmpty()) {
                        continuation.resume(DrowsinessResult.NoFaceDetected)
                        return@addOnSuccessListener
                    }

                    val face = faces.first()

                    // 1. 눈 감김 감지
                    val leftEyeOpenProb = face.leftEyeOpenProbability ?: 0f
                    val rightEyeOpenProb = face.rightEyeOpenProbability ?: 0f

                    val eyesClosed = leftEyeOpenProb < 0.3f && rightEyeOpenProb < 0.3f

                    if (eyesClosed) {
                        if (eyeClosedStartTime == null) {
                            eyeClosedStartTime = System.currentTimeMillis()
                        } else {
                            eyeClosedDuration = System.currentTimeMillis() - eyeClosedStartTime!!
                        }
                    } else {
                        eyeClosedStartTime = null
                        eyeClosedDuration = 0
                    }

                    // 2. 하품 감지
                    val mouthOpenProb = calculateMouthOpenProbability(face)
                    if (mouthOpenProb > 0.7f) {
                        yawnCount++
                    }

                    // 3. 고개 숙임 감지
                    val headEulerAngleX = face.headEulerAngleX
                    if (headEulerAngleX > 20f) {  // 고개를 아래로 숙임
                        headNodCount++
                    }

                    // 졸음 판단
                    val drowsinessLevel = calculateDrowsinessLevel(
                        eyeClosedDuration = eyeClosedDuration,
                        yawnCount = yawnCount,
                        headNodCount = headNodCount
                    )

                    continuation.resume(
                        DrowsinessResult.Success(
                            level = drowsinessLevel,
                            eyeClosedDuration = eyeClosedDuration,
                            yawnCount = yawnCount,
                            headNodCount = headNodCount
                        )
                    )
                }
                .addOnFailureListener { e ->
                    continuation.resume(DrowsinessResult.Error(e.message))
                }
        }
    }

    private fun calculateDrowsinessLevel(
        eyeClosedDuration: Long,
        yawnCount: Int,
        headNodCount: Int
    ): DrowsinessLevel {
        return when {
            eyeClosedDuration > 3000 -> DrowsinessLevel.CRITICAL  // 3초 이상 눈 감음
            eyeClosedDuration > 1500 -> DrowsinessLevel.HIGH      // 1.5초 이상
            yawnCount > 3 -> DrowsinessLevel.MEDIUM               // 하품 3회 이상
            headNodCount > 5 -> DrowsinessLevel.MEDIUM            // 고개 숙임 5회 이상
            eyeClosedDuration > 500 -> DrowsinessLevel.LOW        // 0.5초 이상
            else -> DrowsinessLevel.NORMAL
        }
    }
}

enum class DrowsinessLevel {
    NORMAL,    // 정상
    LOW,       // 약간 졸림
    MEDIUM,    // 졸림
    HIGH,      // 매우 졸림
    CRITICAL   // 위험 (즉시 휴식 필요)
}
```

### 경고 UI

```kotlin
@Composable
fun DrowsinessMonitorScreen(
    viewModel: DrowsinessViewModel = hiltViewModel()
) {
    val drowsinessState by viewModel.drowsinessState.collectAsState()

    Box(modifier = Modifier.fillMaxSize()) {
        // 카메라 프리뷰
        CameraPreview(
            onFrameAnalyzed = { imageProxy ->
                viewModel.analyzeDrowsiness(imageProxy)
            }
        )

        // 경고 오버레이
        when (drowsinessState.level) {
            DrowsinessLevel.CRITICAL -> {
                CriticalWarningOverlay(
                    message = "즉시 휴식을 취하세요!",
                    onStopDriving = { viewModel.triggerEmergencyStop() }
                )
            }

            DrowsinessLevel.HIGH -> {
                HighWarningOverlay(
                    message = "졸음이 감지되었습니다. 휴식을 권장합니다."
                )
            }

            DrowsinessLevel.MEDIUM -> {
                MediumWarningOverlay(
                    message = "피로도가 높아지고 있습니다."
                )
            }

            DrowsinessLevel.LOW -> {
                LowWarningOverlay(
                    message = "조금 피곤해 보입니다."
                )
            }

            DrowsinessLevel.NORMAL -> {
                // 정상 - 표시 없음
            }
        }

        // 상태 정보
        DrowsinessInfo(
            eyeClosedDuration = drowsinessState.eyeClosedDuration,
            yawnCount = drowsinessState.yawnCount,
            headNodCount = drowsinessState.headNodCount
        )
    }
}

@Composable
fun CriticalWarningOverlay(
    message: String,
    onStopDriving: () -> Unit
) {
    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Color.Red.copy(alpha = 0.7f)),
        contentAlignment = Alignment.Center
    ) {
        Column(horizontalAlignment = Alignment.CenterHorizontally) {
            Icon(
                imageVector = Icons.Default.Warning,
                contentDescription = null,
                modifier = Modifier.size(120.dp),
                tint = Color.White
            )

            Text(
                text = message,
                style = MaterialTheme.typography.headlineLarge,
                color = Color.White,
                textAlign = TextAlign.Center
            )

            Spacer(modifier = Modifier.height(32.dp))

            Button(
                onClick = onStopDriving,
                colors = ButtonDefaults.buttonColors(
                    containerColor = Color.White,
                    contentColor = Color.Red
                )
            ) {
                Text("안전 정지", style = MaterialTheme.typography.titleLarge)
            }
        }
    }
}
```

---

## 프로필 관리

### 개요

바디스캔 프로필을 저장하고 관리합니다.

### 기능

#### 1. 프로필 목록

```kotlin
@Composable
fun ProfileListScreen(viewModel: ProfileViewModel = hiltViewModel()) {
    val profiles by viewModel.profiles.collectAsState()

    LazyColumn {
        items(profiles) { profile ->
            ProfileCard(
                profile = profile,
                onSelect = { viewModel.setActiveProfile(profile.profileId) },
                onDelete = { viewModel.deleteProfile(profile.profileId) }
            )
        }

        item {
            AddProfileButton(onClick = { navigateToBodyScan() })
        }
    }
}
```

#### 2. 프로필 상세

```kotlin
@Composable
fun ProfileDetailScreen(
    profileId: String,
    viewModel: ProfileViewModel = hiltViewModel()
) {
    val profile by viewModel.getProfile(profileId).collectAsState()

    profile?.let { p ->
        Column {
            // 신체 정보
            BodyMeasurementSection(bodyScan = p.bodyScan)

            // 차량 세팅
            VehicleSettingsSection(settings = p.vehicleSettings)

            // 적용 버튼
            Button(onClick = {
                navigateToApplySettings(profileId)
            }) {
                Text("이 세팅 적용")
            }

            // 수정 버튼
            OutlinedButton(onClick = {
                navigateToEditProfile(profileId)
            }) {
                Text("수정")
            }
        }
    }
}
```

---

**Last Updated**: 2025-10-03
