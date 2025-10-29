package com.hypermob.android_mvp.ble

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothManager
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.ParcelUuid
import android.util.Log
import java.util.*

/**
 * BLE Scanner and GATT Client
 * Scans for car device advertising with specific UUID and hashkey
 */
@SuppressLint("MissingPermission")
class BleScanner(private val context: Context) {

    private val bluetoothManager: BluetoothManager =
        context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    private val bluetoothAdapter: BluetoothAdapter? = bluetoothManager.adapter
    private val bleScanner = bluetoothAdapter?.bluetoothLeScanner

    private var bluetoothGatt: BluetoothGatt? = null
    private var onDeviceFound: ((String, String) -> Unit)? = null // (deviceAddress, hashkey)
    private var onConnectionEstablished: (() -> Unit)? = null
    private var onWriteSuccess: ((Boolean) -> Unit)? = null

    companion object {
        private const val TAG = "BleScanner"

        // UUIDs from PRD
        val SERVICE_UUID: UUID = UUID.fromString("12345678-1234-5678-1234-56789abcdef0")
        val CHAR_HASHKEY_UUID: UUID = UUID.fromString("12345678-1234-5678-1234-56789abcdef1")
        val CHAR_COMMAND_UUID: UUID = UUID.fromString("12345678-1234-5678-1234-56789abcdef2")

        private const val COMMAND_ACCESS = "ACCESS"
    }

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult?) {
            result?.let { scanResult ->
                val device = scanResult.device
                val scanRecord = scanResult.scanRecord

                Log.d(TAG, "Device found: ${device.address}")

                // Extract hashkey from scan record
                scanRecord?.serviceData?.let { serviceData ->
                    val hashkeyBytes = serviceData[ParcelUuid(SERVICE_UUID)]
                    if (hashkeyBytes != null && hashkeyBytes.size == 8) {
                        val hashkey = hashkeyBytes.joinToString("") { "%02X".format(it) }
                        Log.d(TAG, "Hashkey found: $hashkey")
                        onDeviceFound?.invoke(device.address, hashkey)
                    }
                }
            }
        }

        override fun onScanFailed(errorCode: Int) {
            Log.e(TAG, "BLE Scan failed with error code: $errorCode")
        }
    }

    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt?, status: Int, newState: Int) {
            when (newState) {
                BluetoothGatt.STATE_CONNECTED -> {
                    Log.d(TAG, "Connected to GATT server")
                    gatt?.discoverServices()
                }
                BluetoothGatt.STATE_DISCONNECTED -> {
                    Log.d(TAG, "Disconnected from GATT server")
                    bluetoothGatt?.close()
                    bluetoothGatt = null
                }
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt?, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.d(TAG, "Services discovered")
                onConnectionEstablished?.invoke()
            } else {
                Log.e(TAG, "Service discovery failed: $status")
            }
        }

        override fun onCharacteristicWrite(
            gatt: BluetoothGatt?,
            characteristic: BluetoothGattCharacteristic?,
            status: Int
        ) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.d(TAG, "Characteristic write successful")
                onWriteSuccess?.invoke(true)
            } else {
                Log.e(TAG, "Characteristic write failed: $status")
                onWriteSuccess?.invoke(false)
            }
        }
    }

    /**
     * Start scanning for BLE devices with specific UUID
     */
    fun startScan(
        onFound: (deviceAddress: String, hashkey: String) -> Unit
    ) {
        this.onDeviceFound = onFound

        val scanFilter = ScanFilter.Builder()
            .setServiceUuid(ParcelUuid(SERVICE_UUID))
            .build()

        val scanSettings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()

        bleScanner?.startScan(listOf(scanFilter), scanSettings, scanCallback)
        Log.d(TAG, "BLE scan started")
    }

    /**
     * Stop BLE scanning
     */
    fun stopScan() {
        bleScanner?.stopScan(scanCallback)
        Log.d(TAG, "BLE scan stopped")
    }

    /**
     * Connect to BLE device
     */
    fun connect(
        deviceAddress: String,
        onConnected: () -> Unit
    ) {
        this.onConnectionEstablished = onConnected

        val device: BluetoothDevice? = bluetoothAdapter?.getRemoteDevice(deviceAddress)
        bluetoothGatt = device?.connectGatt(context, false, gattCallback)
        Log.d(TAG, "Connecting to $deviceAddress")
    }

    /**
     * Write "ACCESS" command to car device
     */
    fun writeAccessCommand(onResult: (Boolean) -> Unit) {
        this.onWriteSuccess = onResult

        val service = bluetoothGatt?.getService(SERVICE_UUID)
        val characteristic = service?.getCharacteristic(CHAR_COMMAND_UUID)

        if (characteristic != null) {
            characteristic.value = COMMAND_ACCESS.toByteArray()
            val success = bluetoothGatt?.writeCharacteristic(characteristic) ?: false
            Log.d(TAG, "Writing ACCESS command: $success")
        } else {
            Log.e(TAG, "Command characteristic not found")
            onResult(false)
        }
    }

    /**
     * Disconnect from BLE device
     */
    fun disconnect() {
        bluetoothGatt?.disconnect()
        bluetoothGatt?.close()
        bluetoothGatt = null
        Log.d(TAG, "Disconnected")
    }
}
