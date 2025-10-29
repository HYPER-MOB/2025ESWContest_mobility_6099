package com.hypermob.mydrive3dx.data.hardware

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothManager
import android.bluetooth.le.*
import android.content.Context
import android.os.Build
import android.os.ParcelUuid
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.callbackFlow
import java.util.UUID
import javax.inject.Inject
import javax.inject.Singleton

/**
 * BLE Manager
 *
 * Android Bluetooth LE API를 사용한 BLE 스캔 및 연결 관리자
 */
@Singleton
class BleManager @Inject constructor(
    @ApplicationContext private val context: Context
) {
    private val bluetoothManager: BluetoothManager? =
        context.getSystemService(Context.BLUETOOTH_SERVICE) as? BluetoothManager
    private val bluetoothAdapter: BluetoothAdapter? = bluetoothManager?.adapter

    /**
     * BLE 스캔 결과 상태
     */
    sealed class BleResult {
        object Idle : BleResult()
        object Scanning : BleResult()
        data class DeviceFound(
            val name: String?,
            val address: String,
            val rssi: Int,
            val isVehicle: Boolean
        ) : BleResult()
        data class Connected(
            val deviceName: String?,
            val deviceAddress: String
        ) : BleResult()
        data class Error(val message: String) : BleResult()
    }

    /**
     * Bluetooth 지원 여부 확인
     */
    fun isBluetoothSupported(): Boolean {
        return bluetoothAdapter != null
    }

    /**
     * Bluetooth 활성화 여부 확인
     */
    fun isBluetoothEnabled(): Boolean {
        return bluetoothAdapter?.isEnabled == true
    }

    /**
     * BLE 스캔 시작
     *
     * 차량 BLE 장치를 스캔합니다.
     * 권한: BLUETOOTH_SCAN (Android 12+) or ACCESS_FINE_LOCATION (Android 11-)
     */
    @SuppressLint("MissingPermission")
    fun startScan(): Flow<BleResult> = callbackFlow {
        if (!isBluetoothSupported()) {
            trySend(BleResult.Error("Bluetooth를 지원하지 않는 기기입니다"))
            close()
            return@callbackFlow
        }

        if (!isBluetoothEnabled()) {
            trySend(BleResult.Error("Bluetooth가 비활성화되어 있습니다"))
            close()
            return@callbackFlow
        }

        val bluetoothLeScanner = bluetoothAdapter?.bluetoothLeScanner
        if (bluetoothLeScanner == null) {
            trySend(BleResult.Error("BLE 스캐너를 초기화할 수 없습니다"))
            close()
            return@callbackFlow
        }

        trySend(BleResult.Scanning)

        val scanSettings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()

        val scanCallback = object : ScanCallback() {
            override fun onScanResult(callbackType: Int, result: ScanResult) {
                super.onScanResult(callbackType, result)

                val device = result.device
                val deviceName = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                    try {
                        device.name
                    } catch (e: SecurityException) {
                        "Unknown"
                    }
                } else {
                    device.name
                }

                val isVehicle = isVehicleDevice(deviceName, device.address)

                trySend(
                    BleResult.DeviceFound(
                        name = deviceName,
                        address = device.address,
                        rssi = result.rssi,
                        isVehicle = isVehicle
                    )
                )

                // 차량 장치를 찾으면 자동으로 스캔 중지
                if (isVehicle) {
                    bluetoothLeScanner.stopScan(this)
                }
            }

            override fun onScanFailed(errorCode: Int) {
                super.onScanFailed(errorCode)
                val errorMessage = when (errorCode) {
                    SCAN_FAILED_ALREADY_STARTED -> "스캔이 이미 진행 중입니다"
                    SCAN_FAILED_APPLICATION_REGISTRATION_FAILED -> "앱 등록에 실패했습니다"
                    SCAN_FAILED_FEATURE_UNSUPPORTED -> "BLE를 지원하지 않습니다"
                    SCAN_FAILED_INTERNAL_ERROR -> "내부 오류가 발생했습니다"
                    else -> "알 수 없는 오류 (코드: $errorCode)"
                }
                trySend(BleResult.Error(errorMessage))
            }
        }

        try {
            bluetoothLeScanner.startScan(null, scanSettings, scanCallback)
        } catch (e: SecurityException) {
            trySend(BleResult.Error("Bluetooth 권한이 필요합니다"))
        }

        awaitClose {
            try {
                bluetoothLeScanner.stopScan(scanCallback)
            } catch (e: Exception) {
                // Ignore
            }
        }
    }

    /**
     * 차량 장치인지 확인
     *
     * 실제 구현에서는 차량 BLE 장치의 특정 이름 패턴이나 UUID를 확인해야 합니다.
     */
    private fun isVehicleDevice(deviceName: String?, deviceAddress: String): Boolean {
        // TODO: 실제 차량 BLE 장치의 이름 패턴이나 서비스 UUID 확인
        // 예: "MyDrive_Vehicle_*" 패턴 또는 특정 서비스 UUID 포함 여부

        if (deviceName == null) return false

        // 시뮬레이션: 이름에 "Vehicle", "Car", "MyDrive" 등이 포함되면 차량으로 간주
        val vehicleKeywords = listOf("vehicle", "car", "mydrive", "auto", "smart")
        return vehicleKeywords.any { keyword ->
            deviceName.lowercase().contains(keyword)
        }
    }

    /**
     * 차량 BLE 연결 검증
     *
     * 실제 구현에서는 GATT 서버에 연결하여 차량 인증을 수행해야 합니다.
     */
    fun verifyVehicleConnection(deviceAddress: String, rentalId: String): Boolean {
        // TODO: GATT 서버 연결 및 차량 인증
        // 1. BluetoothGatt.connect() 호출
        // 2. 서비스 검색 (discoverServices)
        // 3. 특정 Characteristic에 rentalId 쓰기
        // 4. 응답 읽기 및 검증

        // 지금은 시뮬레이션으로 항상 true 반환
        return deviceAddress.isNotEmpty()
    }

    /**
     * BLE Advertising 시작
     *
     * 이 앱이 BLE Peripheral로 동작하여 인증 키를 Broadcast
     *
     * @param authKey 인증 키
     * @return Advertising 결과 Flow
     */
    @SuppressLint("MissingPermission")
    fun startAdvertising(authKey: String): Flow<AdvertiseResult> = callbackFlow {
        if (!isBluetoothSupported()) {
            trySend(AdvertiseResult.Error("Bluetooth를 지원하지 않는 기기입니다"))
            close()
            return@callbackFlow
        }

        if (!isBluetoothEnabled()) {
            trySend(AdvertiseResult.Error("Bluetooth가 비활성화되어 있습니다"))
            close()
            return@callbackFlow
        }

        val bluetoothLeAdvertiser = bluetoothAdapter?.bluetoothLeAdvertiser
        if (bluetoothLeAdvertiser == null) {
            trySend(AdvertiseResult.Error("BLE Advertising을 지원하지 않는 기기입니다"))
            close()
            return@callbackFlow
        }

        trySend(AdvertiseResult.Starting)

        // Advertising 설정
        val settings = AdvertiseSettings.Builder()
            .setAdvertiseMode(AdvertiseSettings.ADVERTISE_MODE_LOW_LATENCY) // 빠른 응답
            .setTxPowerLevel(AdvertiseSettings.ADVERTISE_TX_POWER_HIGH) // 강한 신호
            .setConnectable(false) // 연결 불필요 (Broadcast만)
            .setTimeout(0) // 무제한 (수동 중지할 때까지)
            .build()

        // MyDrive3DX 서비스 UUID
        val serviceUuid = ParcelUuid(UUID.fromString("0000FFF0-0000-1000-8000-00805F9B34FB"))

        // Advertising 데이터 (서비스 UUID + 인증 키)
        val data = AdvertiseData.Builder()
            .addServiceUuid(serviceUuid)
            .addServiceData(serviceUuid, authKey.toByteArray())
            .setIncludeDeviceName(true)
            .setIncludeTxPowerLevel(false)
            .build()

        // Scan Response 데이터 (추가 정보)
        val scanResponse = AdvertiseData.Builder()
            .setIncludeDeviceName(true)
            .build()

        val callback = object : AdvertiseCallback() {
            override fun onStartSuccess(settingsInEffect: AdvertiseSettings?) {
                super.onStartSuccess(settingsInEffect)
                trySend(AdvertiseResult.Broadcasting(authKey))
            }

            override fun onStartFailure(errorCode: Int) {
                super.onStartFailure(errorCode)
                val errorMessage = when (errorCode) {
                    ADVERTISE_FAILED_DATA_TOO_LARGE -> "데이터가 너무 큽니다"
                    ADVERTISE_FAILED_TOO_MANY_ADVERTISERS -> "동시 Advertising 제한 초과"
                    ADVERTISE_FAILED_ALREADY_STARTED -> "이미 Advertising 중입니다"
                    ADVERTISE_FAILED_INTERNAL_ERROR -> "내부 오류가 발생했습니다"
                    ADVERTISE_FAILED_FEATURE_UNSUPPORTED -> "BLE Advertising을 지원하지 않습니다"
                    else -> "알 수 없는 오류 (코드: $errorCode)"
                }
                trySend(AdvertiseResult.Error(errorMessage))
            }
        }

        try {
            bluetoothLeAdvertiser.startAdvertising(settings, data, scanResponse, callback)
        } catch (e: SecurityException) {
            trySend(AdvertiseResult.Error("Bluetooth 권한이 필요합니다"))
        }

        awaitClose {
            try {
                bluetoothLeAdvertiser.stopAdvertising(callback)
            } catch (e: Exception) {
                // Ignore
            }
        }
    }

    /**
     * BLE Advertising 중지
     *
     * 현재는 Flow가 close될 때 자동으로 중지됨
     */
    fun stopAdvertising() {
        // Flow의 awaitClose에서 처리됨
    }

    // === android_mvp에서 추가된 MFA 인증 기능 ===

    private var currentGatt: android.bluetooth.BluetoothGatt? = null

    /**
     * BLE 기기 스캔 (MFA용 - android_mvp 통합)
     * SERVICE_UUID로 필터링하여 인증 기기만 검색
     */
    @SuppressLint("MissingPermission")
    fun scanForDevices(): Flow<BluetoothDevice> = callbackFlow {
        val bluetoothLeScanner = bluetoothAdapter?.bluetoothLeScanner
        if (bluetoothLeScanner == null) {
            close(Exception("BLE scanner not available"))
            return@callbackFlow
        }

        val scanCallback = object : ScanCallback() {
            override fun onScanResult(callbackType: Int, result: ScanResult) {
                trySend(result.device)
            }

            override fun onScanFailed(errorCode: Int) {
                close(Exception("Scan failed with error: $errorCode"))
            }
        }

        val filter = ScanFilter.Builder()
            .setServiceUuid(ParcelUuid(SERVICE_UUID))
            .build()

        val settings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()

        bluetoothLeScanner.startScan(listOf(filter), settings, scanCallback)

        awaitClose {
            bluetoothLeScanner.stopScan(scanCallback)
        }
    }

    /**
     * GATT 연결 및 "ACCESS" 문자열 쓰기 (MFA용 - android_mvp 통합)
     * android_mvp의 AuthenticationActivity.kt 로직 기반
     *
     * @param device 연결할 BLE 기기
     * @return 성공/실패 Flow
     */
    @SuppressLint("MissingPermission")
    fun connectAndWriteAccess(device: BluetoothDevice): Flow<Boolean> = callbackFlow {
        val gattCallback = object : android.bluetooth.BluetoothGattCallback() {
            override fun onConnectionStateChange(
                gatt: android.bluetooth.BluetoothGatt,
                status: Int,
                newState: Int
            ) {
                when (newState) {
                    android.bluetooth.BluetoothProfile.STATE_CONNECTED -> {
                        android.util.Log.d("BleManager", "Connected to GATT server")
                        // 서비스 검색
                        gatt.discoverServices()
                    }
                    android.bluetooth.BluetoothProfile.STATE_DISCONNECTED -> {
                        android.util.Log.d("BleManager", "Disconnected from GATT server")
                        close()
                    }
                }
            }

            override fun onServicesDiscovered(gatt: android.bluetooth.BluetoothGatt, status: Int) {
                if (status == android.bluetooth.BluetoothGatt.GATT_SUCCESS) {
                    val service = gatt.getService(SERVICE_UUID)
                    val characteristic = service?.getCharacteristic(CHARACTERISTIC_UUID)

                    if (characteristic != null) {
                        // "ACCESS" 문자열을 바이트 배열로 변환하여 쓰기
                        characteristic.value = "ACCESS".toByteArray(Charsets.UTF_8)
                        characteristic.writeType = android.bluetooth.BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE

                        val success = gatt.writeCharacteristic(characteristic)
                        android.util.Log.d("BleManager", "Write ACCESS: $success")

                        if (success) {
                            trySend(true)
                        } else {
                            trySend(false)
                            close(Exception("Failed to write ACCESS"))
                        }
                    } else {
                        android.util.Log.e("BleManager", "Characteristic not found")
                        trySend(false)
                        close(Exception("Characteristic not found"))
                    }
                } else {
                    android.util.Log.e("BleManager", "Service discovery failed: $status")
                    trySend(false)
                    close(Exception("Service discovery failed"))
                }
            }

            override fun onCharacteristicWrite(
                gatt: android.bluetooth.BluetoothGatt,
                characteristic: android.bluetooth.BluetoothGattCharacteristic,
                status: Int
            ) {
                if (status == android.bluetooth.BluetoothGatt.GATT_SUCCESS) {
                    android.util.Log.d("BleManager", "ACCESS written successfully")
                    trySend(true)
                } else {
                    android.util.Log.e("BleManager", "Failed to write ACCESS: $status")
                    trySend(false)
                }
                // 쓰기 완료 후 연결 종료
                gatt.disconnect()
                close()
            }
        }

        // TRANSPORT_LE로 BLE 전용 연결 (android_mvp 로직)
        currentGatt = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            device.connectGatt(context, false, gattCallback, android.bluetooth.BluetoothDevice.TRANSPORT_LE)
        } else {
            device.connectGatt(context, false, gattCallback)
        }

        awaitClose {
            currentGatt?.close()
            currentGatt = null
        }
    }

    /**
     * GATT 명령 전송 (MFA용 - android_mvp 통합)
     */
    @SuppressLint("MissingPermission")
    fun sendCommand(command: String): Flow<Boolean> = callbackFlow {
        val gatt = currentGatt ?: run {
            close(Exception("No active GATT connection"))
            return@callbackFlow
        }

        val service = gatt.getService(SERVICE_UUID)
        val commandChar = service?.getCharacteristic(CHAR_COMMAND_UUID)

        if (commandChar != null) {
            commandChar.setValue(command)
            val success = gatt.writeCharacteristic(commandChar)
            trySend(success)
        } else {
            trySend(false)
        }

        awaitClose { }
    }

    /**
     * 연결 종료
     */
    @SuppressLint("MissingPermission")
    fun disconnect() {
        currentGatt?.disconnect()
        currentGatt?.close()
        currentGatt = null
    }

    companion object {
        // android_mvp에서 사용하던 UUID (MFA 인증용)
        // AuthenticationActivity.kt의 실제 UUID 사용
        val SERVICE_UUID: UUID = UUID.fromString("12345678-0000-1000-8000-111033441122")
        val CHARACTERISTIC_UUID: UUID = UUID.fromString("c0de0001-0000-1000-8000-000000000001")

        // Deprecated - 이전 버전 (사용 안 함)
        @Deprecated("Use SERVICE_UUID and CHARACTERISTIC_UUID instead")
        val CHAR_HASHKEY_UUID: UUID = UUID.fromString("12345678-1234-5678-1234-56789abcdef1")
        @Deprecated("Use CHARACTERISTIC_UUID instead")
        val CHAR_COMMAND_UUID: UUID = UUID.fromString("12345678-1234-5678-1234-56789abcdef2")

        private const val SCAN_FAILED_ALREADY_STARTED = 1
        private const val SCAN_FAILED_APPLICATION_REGISTRATION_FAILED = 2
        private const val SCAN_FAILED_FEATURE_UNSUPPORTED = 4
        private const val SCAN_FAILED_INTERNAL_ERROR = 3

        private const val ADVERTISE_FAILED_DATA_TOO_LARGE = 1
        private const val ADVERTISE_FAILED_TOO_MANY_ADVERTISERS = 2
        private const val ADVERTISE_FAILED_ALREADY_STARTED = 3
        private const val ADVERTISE_FAILED_INTERNAL_ERROR = 4
        private const val ADVERTISE_FAILED_FEATURE_UNSUPPORTED = 5
    }
}

/**
 * BLE Advertising 결과
 */
sealed class AdvertiseResult {
    object Starting : AdvertiseResult()
    data class Broadcasting(val authKey: String) : AdvertiseResult()
    data class Error(val message: String) : AdvertiseResult()
}
