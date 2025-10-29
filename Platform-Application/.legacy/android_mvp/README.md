# HYPERMOB MFA Android MVP

Android MVP application for multi-factor authentication (Face → BLE → NFC).

## 📋 Overview

This Android app implements the **HYPERMOB MFA Authentication System** as specified in the PRD. It provides a complete authentication flow using:

1. **Face Recognition** - Camera-based face image registration
2. **BLE (Bluetooth Low Energy)** - Scanning and connecting to car device
3. **NFC (Near Field Communication)** - Final authentication with NFC tag

---

## 🏗️ Architecture

### Package Structure

```
com.hypermob.android_mvp/
├── data/
│   ├── api/
│   │   ├── AuthService.kt          # Retrofit API interface
│   │   └── RetrofitClient.kt       # Retrofit singleton
│   └── model/
│       └── ApiModels.kt            # Data classes for API requests/responses
├── ui/
│   ├── MainActivity.kt             # Main screen with state machine
│   ├── FaceRegisterActivity.kt    # Face registration (CameraX)
│   ├── NfcRegisterActivity.kt     # NFC tag registration
│   ├── BleAuthActivity.kt         # BLE authentication
│   └── NfcAuthActivity.kt         # NFC final authentication
├── ble/
│   └── BleScanner.kt              # BLE scanning and GATT client
├── nfc/
│   └── NfcReader.kt               # NFC tag UID reader
└── util/
    └── PreferenceManager.kt       # SharedPreferences wrapper
```

---

## 🔄 Authentication Flow

### Registration Flow (One-time)

```
1. MainActivity → FaceRegisterActivity
   - Capture face image with CameraX
   - Upload to POST /auth/face
   - Save user_id and face_id

2. MainActivity → NfcRegisterActivity
   - Read NFC tag UID
   - Upload to POST /auth/nfc
   - Save nfc_uid
```

### Authentication Flow (Repeatable)

```
1. MainActivity → BleAuthActivity
   - Fetch hashkey from GET /auth/ble?user_id={id}
   - Scan for car BLE device with matching UUID
   - Verify hashkey matches
   - Connect and write "ACCESS" command

2. BleAuthActivity → NfcAuthActivity
   - Read NFC tag UID
   - Verify with POST /auth/nfc/verify
   - Server checks:
     ✓ NFC UID matches registered UID
     ✓ Face auth completed (from car)
     ✓ BLE auth completed
   - Show MFA_SUCCESS or MFA_FAILED
```

---

## 🛠️ Technologies Used

| Component | Library | Version |
|-----------|---------|---------|
| Language | Kotlin | 1.9+ |
| UI | ViewBinding | - |
| Camera | CameraX | 1.3.0 |
| Networking | Retrofit 2 | 2.9.0 |
| JSON | Gson | 2.9.0 |
| Async | Coroutines | 1.7.3 |
| BLE | Android Bluetooth API | - |
| NFC | Android NFC API | - |

---

## 📱 Key Features

### 1. State Machine (MainActivity)

- **NOT_REGISTERED**: User must register face first
- **FACE_REGISTERED**: User must register NFC tag
- **FULLY_REGISTERED**: Ready for authentication

### 2. Face Registration (FaceRegisterActivity)

- Uses **CameraX** for camera preview and capture
- Front camera with mirror effect
- Converts to JPEG and uploads via multipart/form-data
- Saves `user_id` and `face_id` to SharedPreferences

### 3. NFC Registration (NfcRegisterActivity)

- Enables NFC foreground dispatch
- Reads ISO14443A tag UID (14 hex characters)
- Validates UID format
- Uploads to server and saves locally

### 4. BLE Authentication (BleAuthActivity)

- Fetches BLE hashkey from server
- Scans for car device with UUID: `12345678-1234-5678-1234-56789abcdef0`
- Compares scanned hashkey with server hashkey
- Connects via GATT and writes "ACCESS" to command characteristic

### 5. NFC Final Authentication (NfcAuthActivity)

- Reads NFC tag UID
- Sends to server for final verification
- Server validates all 3 factors (Face, BLE, NFC)
- Shows success/failure with color indicators

---

## ⚙️ Configuration

### API Base URL

Update the API Gateway URL in `RetrofitClient.kt`:

```kotlin
private const val BASE_URL = "https://your-api-id.execute-api.us-east-1.amazonaws.com/v1/"
```

### BLE UUIDs (from PRD)

Defined in `BleScanner.kt`:

```kotlin
val SERVICE_UUID = UUID.fromString("12345678-1234-5678-1234-56789abcdef0")
val CHAR_HASHKEY_UUID = UUID.fromString("12345678-1234-5678-1234-56789abcdef1")
val CHAR_COMMAND_UUID = UUID.fromString("12345678-1234-5678-1234-56789abcdef2")
```

---

## 🔐 Permissions

Required permissions (auto-requested at runtime):

- `CAMERA` - Face image capture
- `NFC` - NFC tag reading
- `BLUETOOTH` / `BLUETOOTH_ADMIN` (API < 31)
- `BLUETOOTH_SCAN` / `BLUETOOTH_CONNECT` (API ≥ 31)
- `ACCESS_FINE_LOCATION` - Required for BLE scanning

---

## 🚀 Building and Running

### Prerequisites

- Android Studio Arctic Fox or later
- Android SDK 24+ (minSdk)
- Target SDK 35
- Physical device with NFC and BLE support

### Build Steps

1. Clone the repository
2. Open in Android Studio
3. Update `BASE_URL` in `RetrofitClient.kt`
4. Sync Gradle
5. Run on physical device (NFC/BLE not available on emulator)

### Gradle Build

```bash
./gradlew assembleDebug
```

---

## 📊 API Endpoints Used

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/health` | GET | Server health check |
| `/auth/face` | POST | Register face image |
| `/auth/nfc` | POST | Register NFC UID |
| `/auth/ble` | GET | Get BLE hashkey |
| `/auth/session` | GET | Get auth session (for car) |
| `/auth/result` | POST | Report auth result (from car) |
| `/auth/nfc/verify` | POST | Final NFC verification |

---

## 🧪 Testing

### Manual Testing Checklist

- [ ] Face registration captures and uploads image
- [ ] NFC registration reads and saves UID
- [ ] BLE scan finds car device
- [ ] BLE hashkey comparison works
- [ ] BLE GATT write succeeds
- [ ] NFC final auth succeeds with valid credentials
- [ ] NFC final auth fails with mismatched credentials
- [ ] State machine transitions work correctly
- [ ] Clear data resets to initial state

### Testing Without Car Device

You can test the app without a physical car device:

1. **Face & NFC Registration**: Works independently
2. **BLE Authentication**: Will timeout if no device found
3. **Mock Server**: Use Postman/curl to simulate API responses

---

## 📝 Known Limitations

1. **BLE Scanning**: Requires actual car device for full testing
2. **Face Recognition**: Server-side only (no on-device ML)
3. **NFC**: Requires physical NFC tag
4. **Car ID**: Hardcoded to "CAR123"
5. **No Offline Mode**: Requires internet connection

---

## 🔧 Future Enhancements

- [ ] Add on-device face liveness detection
- [ ] Implement retry logic for network failures
- [ ] Add loading animations
- [ ] Support multiple car IDs
- [ ] Add authentication history
- [ ] Implement biometric (fingerprint) as backup
- [ ] Add dark mode support
- [ ] Localization (Korean/English)

---

## 📄 License

Copyright © 2025 HYPERMOB. All rights reserved.

---

## 👥 Development Team

- **Backend**: AWS Lambda + API Gateway + RDS MySQL
- **Android**: Kotlin + CameraX + Retrofit + BLE/NFC APIs
- **Documentation**: PRD-based implementation

---

## 📞 Support

For issues or questions:
- Check CloudWatch Logs for server errors
- Check Logcat for Android errors
- Refer to `.docs/mvp_prd.md` for specification details
