package com.hypermob.android_mvp.ui

import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Intent
import android.nfc.NfcAdapter
import android.os.Bundle
import android.os.ParcelUuid
import android.util.Log
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.hypermob.android_mvp.databinding.ActivityAuthenticationBinding
import com.hypermob.android_mvp.util.PreferenceManager
import java.util.UUID

/**
 * Unified Authentication Activity
 * - Auto-scans for BLE device with specific Service UUID
 * - Automatically writes "ACCESS" in hex to the characteristic
 * - NFC HCE is already active in background (via NfcHceService)
 */
class AuthenticationActivity : AppCompatActivity() {

    private lateinit var binding: ActivityAuthenticationBinding
    private lateinit var prefManager: PreferenceManager

    private var bluetoothAdapter: BluetoothAdapter? = null
    private var isScanning = false

    companion object {
        private const val TAG = "AuthenticationActivity"

        // BLE Service UUID (차량에서 브로드캐스트하는 UUID)
        private val SERVICE_UUID = UUID.fromString("12345678-0000-1000-8000-111033441122")

        // Characteristic UUID (차량에서 기본 생성하는 Characteristic)
        private val CHARACTERISTIC_UUID = UUID.fromString("c0de0001-0000-1000-8000-000000000001")
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityAuthenticationBinding.inflate(layoutInflater)
        setContentView(binding.root)

        prefManager = PreferenceManager(this)

        setupBluetooth()
        setupUI()
        checkNfcStatus()
    }

    private fun setupBluetooth() {
        val bluetoothManager = getSystemService(BLUETOOTH_SERVICE) as BluetoothManager
        bluetoothAdapter = bluetoothManager.adapter

        if (bluetoothAdapter == null) {
            Toast.makeText(this, "Bluetooth not supported", Toast.LENGTH_SHORT).show()
            finish()
            return
        }

        if (!bluetoothAdapter!!.isEnabled) {
            Toast.makeText(this, "Please enable Bluetooth", Toast.LENGTH_LONG).show()
            // Optionally: startActivityForResult(Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE), ...)
        }
    }

    private fun setupUI() {
        val userId = prefManager.userId
        val nfcUid = prefManager.nfcUid

        binding.tvUserId.text = "User ID: ${userId ?: "Not registered"}"
        binding.tvNfcUid.text = "NFC UID: ${nfcUid ?: "Not registered"}"

        binding.btnStartScan.setOnClickListener {
            if (isScanning) {
                stopBleScan()
            } else {
                startBleScan()
            }
        }

        binding.btnBack.setOnClickListener {
            finish()
        }

        // Auto-start BLE scan
        startBleScan()
    }

    private fun checkNfcStatus() {
        val nfcAdapter = NfcAdapter.getDefaultAdapter(this)

        if (nfcAdapter == null) {
            binding.tvNfcStatus.text = "NFC Status: Not supported"
            return
        }

        if (!nfcAdapter.isEnabled) {
            binding.tvNfcStatus.text = "NFC Status: Disabled (Please enable in settings)"
            Toast.makeText(this, "Please enable NFC in settings", Toast.LENGTH_LONG).show()
        } else {
            val nfcUid = prefManager.nfcUid
            binding.tvNfcStatus.text = "NFC Status: Active (Ready to tap)"

            Log.d(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
            Log.d(TAG, "✓ NFC HCE is ACTIVE and READY")
            Log.d(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
            Log.d(TAG, "NFC UID: $nfcUid")
            Log.d(TAG, "AID: F0010203040506")
            Log.d(TAG, "Service: com.hypermob.android_mvp.service.NfcHceService")
            Log.d(TAG, "")
            Log.d(TAG, "🧪 TO TEST:")
            Log.d(TAG, "1. Tap with another NFC phone/reader")
            Log.d(TAG, "2. Watch for 'Received APDU' log below")
            Log.d(TAG, "3. Expected response: $nfcUid + 9000")
            Log.d(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
        }
    }

    private fun startBleScan() {
        if (isScanning) {
            Log.w(TAG, "Already scanning")
            return
        }

        val bluetoothLeScanner = bluetoothAdapter?.bluetoothLeScanner
        if (bluetoothLeScanner == null) {
            Toast.makeText(this, "BLE Scanner not available", Toast.LENGTH_SHORT).show()
            return
        }

        // No filter - scan all BLE devices
        // (차량이 Service UUID를 advertise하지 않을 수 있음)
        val scanSettings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()

        isScanning = true
        binding.progressBar.visibility = View.VISIBLE
        binding.tvBleStatus.text = "BLE Status: Scanning for all BLE devices..."
        binding.btnStartScan.text = "Stop Scanning"

        Log.d(TAG, "Starting BLE scan (no filter)")

        // Scan without filter to find all devices
        bluetoothLeScanner.startScan(null, scanSettings, scanCallback)
    }

    private fun stopBleScan() {
        if (!isScanning) return

        bluetoothAdapter?.bluetoothLeScanner?.stopScan(scanCallback)
        isScanning = false
        binding.progressBar.visibility = View.GONE
        binding.tvBleStatus.text = "BLE Status: Scan stopped"
        binding.btnStartScan.text = "Start BLE Scan"

        Log.d(TAG, "BLE scan stopped")
    }

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            super.onScanResult(callbackType, result)

            val device = result.device
            val deviceName = device.name ?: "Unknown"
            val rssi = result.rssi

            // Log all devices found (for debugging)
            Log.d(TAG, "Found BLE device: ${device.address}, name=$deviceName, RSSI=$rssi dBm")

            // Check if device advertises our Service UUID
            val serviceUuids = result.scanRecord?.serviceUuids
            if (serviceUuids != null) {
                Log.d(TAG, "  -> Advertised services: ${serviceUuids.joinToString(", ")}")

                // If found our service, auto-connect
                if (serviceUuids.any { it.uuid == SERVICE_UUID }) {
                    Log.d(TAG, "  -> MATCHED! Vehicle found with Service UUID")
                    stopBleScan()
                    binding.tvBleStatus.text = "BLE Status: Vehicle found! ($deviceName)"
                    connectAndWriteAccess(device)
                    return
                }
            }

            // Update status to show we're finding devices
            runOnUiThread {
                binding.tvBleStatus.text = "BLE Status: Found $deviceName (${device.address}), still scanning..."
            }
        }

        override fun onScanFailed(errorCode: Int) {
            super.onScanFailed(errorCode)
            Log.e(TAG, "BLE scan failed with error code: $errorCode")

            stopBleScan()
            binding.tvBleStatus.text = "BLE Status: Scan failed (Error: $errorCode)"
            Toast.makeText(this@AuthenticationActivity, "BLE scan failed", Toast.LENGTH_SHORT).show()
        }
    }

    private fun connectAndWriteAccess(device: android.bluetooth.BluetoothDevice) {
        binding.tvBleStatus.text = "BLE Status: Connecting to ${device.address}..."
        binding.progressBar.visibility = View.VISIBLE

        Log.d(TAG, "Connecting with BLE transport (TRANSPORT_LE)...")

        // GATT connection - MUST specify TRANSPORT_LE for BLE devices
        val gattCallback = object : android.bluetooth.BluetoothGattCallback() {
            override fun onConnectionStateChange(
                gatt: android.bluetooth.BluetoothGatt,
                status: Int,
                newState: Int
            ) {
                if (newState == android.bluetooth.BluetoothProfile.STATE_CONNECTED) {
                    Log.d(TAG, "Connected to GATT server, discovering services...")
                    runOnUiThread {
                        binding.tvBleStatus.text = "BLE Status: Connected, discovering services..."
                    }
                    gatt.discoverServices()
                } else if (newState == android.bluetooth.BluetoothProfile.STATE_DISCONNECTED) {
                    Log.d(TAG, "Disconnected from GATT server")
                    runOnUiThread {
                        binding.tvBleStatus.text = "BLE Status: Disconnected"
                        binding.progressBar.visibility = View.GONE
                    }
                    gatt.close()
                }
            }

            override fun onServicesDiscovered(gatt: android.bluetooth.BluetoothGatt, status: Int) {
                if (status == android.bluetooth.BluetoothGatt.GATT_SUCCESS) {
                    Log.d(TAG, "Services discovered")

                    val service = gatt.getService(SERVICE_UUID)
                    if (service == null) {
                        Log.e(TAG, "Service not found: $SERVICE_UUID")
                        runOnUiThread {
                            binding.tvBleStatus.text = "BLE Status: Service not found"
                            binding.progressBar.visibility = View.GONE
                        }
                        gatt.disconnect()
                        return
                    }

                    val characteristic = service.getCharacteristic(CHARACTERISTIC_UUID)
                    if (characteristic == null) {
                        Log.e(TAG, "Characteristic not found: $CHARACTERISTIC_UUID")
                        runOnUiThread {
                            binding.tvBleStatus.text = "BLE Status: Characteristic not found"
                            binding.progressBar.visibility = View.GONE
                        }
                        gatt.disconnect()
                        return
                    }

                    // Log characteristic properties
                    val properties = characteristic.properties
                    Log.d(TAG, "Characteristic properties: 0x${properties.toString(16)}")

                    val supportsWrite = (properties and android.bluetooth.BluetoothGattCharacteristic.PROPERTY_WRITE) != 0
                    val supportsWriteNoResponse = (properties and android.bluetooth.BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE) != 0
                    val supportsSignedWrite = (properties and android.bluetooth.BluetoothGattCharacteristic.PROPERTY_SIGNED_WRITE) != 0

                    Log.d(TAG, "  - WRITE: $supportsWrite")
                    Log.d(TAG, "  - WRITE_NO_RESPONSE: $supportsWriteNoResponse")
                    Log.d(TAG, "  - SIGNED_WRITE: $supportsSignedWrite")

                    // 페어링 없이 쓰기 위해 우선순위에 따라 write type 선택
                    when {
                        supportsWriteNoResponse -> {
                            characteristic.writeType = android.bluetooth.BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
                            Log.d(TAG, "Using WRITE_TYPE_NO_RESPONSE (no pairing needed)")
                        }
                        supportsWrite -> {
                            characteristic.writeType = android.bluetooth.BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
                            Log.d(TAG, "Using WRITE_TYPE_DEFAULT (may need pairing)")
                        }
                        supportsSignedWrite -> {
                            characteristic.writeType = android.bluetooth.BluetoothGattCharacteristic.WRITE_TYPE_SIGNED
                            Log.d(TAG, "Using WRITE_TYPE_SIGNED (needs pairing)")
                        }
                        else -> {
                            Log.e(TAG, "Characteristic doesn't support any write operation!")
                            runOnUiThread {
                                binding.tvBleStatus.text = "BLE Status: Write not supported"
                                binding.progressBar.visibility = View.GONE
                            }
                            gatt.disconnect()
                            return
                        }
                    }

                    // Write "ACCESS"
                    val accessBytes = "ACCESS".toByteArray(Charsets.UTF_8)
                    Log.d(TAG, "Writing 'ACCESS' (${accessBytes.size} bytes): ${accessBytes.joinToString(" ") { "0x%02x".format(it) }}")

                    runOnUiThread {
                        binding.tvBleStatus.text = "BLE Status: Writing 'ACCESS'..."
                    }

                    characteristic.value = accessBytes
                    val writeSuccess = gatt.writeCharacteristic(characteristic)
                    Log.d(TAG, "writeCharacteristic returned: $writeSuccess")
                } else {
                    Log.e(TAG, "Service discovery failed with status: $status")
                    runOnUiThread {
                        binding.tvBleStatus.text = "BLE Status: Service discovery failed"
                        binding.progressBar.visibility = View.GONE
                    }
                    gatt.disconnect()
                }
            }

            @Deprecated("Deprecated in Android 13")
            override fun onCharacteristicWrite(
                gatt: android.bluetooth.BluetoothGatt,
                characteristic: android.bluetooth.BluetoothGattCharacteristic,
                status: Int
            ) {
                if (status == android.bluetooth.BluetoothGatt.GATT_SUCCESS) {
                    Log.d(TAG, "'ACCESS' written successfully!")
                    runOnUiThread {
                        binding.tvBleStatus.text = "BLE Status: 'ACCESS' sent successfully! ✓"
                        binding.progressBar.visibility = View.GONE
                        Toast.makeText(this@AuthenticationActivity, "BLE Authentication Complete!", Toast.LENGTH_SHORT).show()
                    }
                } else {
                    Log.e(TAG, "Failed to write characteristic, status: $status")
                    runOnUiThread {
                        binding.tvBleStatus.text = "BLE Status: Write failed (Status: $status)"
                        binding.progressBar.visibility = View.GONE
                    }
                }

                // Disconnect after writing
                gatt.disconnect()
            }
        }

        // Connect with TRANSPORT_LE to force BLE connection (not Classic Bluetooth)
        // This prevents pairing dialogs for BLE-only devices
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
            device.connectGatt(
                this,
                false,  // autoConnect = false (connect immediately)
                gattCallback,
                android.bluetooth.BluetoothDevice.TRANSPORT_LE  // ← KEY: Force BLE transport
            )
        } else {
            // Fallback for older devices
            device.connectGatt(this, false, gattCallback)
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        stopBleScan()
    }
}
