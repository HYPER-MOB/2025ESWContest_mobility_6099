package com.hypermob.android_mvp.ui

import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.hypermob.android_mvp.ble.BleScanner
import com.hypermob.android_mvp.data.api.RetrofitClient
import com.hypermob.android_mvp.databinding.ActivityBleAuthBinding
import com.hypermob.android_mvp.util.PreferenceManager
import kotlinx.coroutines.launch

/**
 * BLE Authentication Activity
 *
 * Flow:
 * 1. Get BLE hashkey from server (GET /auth/ble)
 * 2. Scan for car device advertising with matching UUID
 * 3. Compare hashkey from scan with server hashkey
 * 4. Connect to car device
 * 5. Write "ACCESS" command
 * 6. Proceed to NFC authentication
 */
class BleAuthActivity : AppCompatActivity() {

    private lateinit var binding: ActivityBleAuthBinding
    private lateinit var prefManager: PreferenceManager
    private lateinit var bleScanner: BleScanner

    private var serverHashkey: String? = null
    private var deviceAddress: String? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityBleAuthBinding.inflate(layoutInflater)
        setContentView(binding.root)

        prefManager = PreferenceManager(this)
        bleScanner = BleScanner(this)

        setupUI()
        fetchBleHashkey()
    }

    override fun onDestroy() {
        super.onDestroy()
        bleScanner.stopScan()
        bleScanner.disconnect()
    }

    private fun setupUI() {
        binding.btnStartScan.setOnClickListener {
            startBleScan()
        }

        binding.btnCancel.setOnClickListener {
            finish()
        }
    }

    private fun fetchBleHashkey() {
        val userId = prefManager.userId
        if (userId == null) {
            Toast.makeText(this, "User not registered", Toast.LENGTH_SHORT).show()
            finish()
            return
        }

        binding.tvStatus.text = "Fetching BLE hashkey..."
        binding.btnStartScan.isEnabled = false

        lifecycleScope.launch {
            try {
                val response = RetrofitClient.authService.getBleHashkey(userId, "CAR123")

                if (response.isSuccessful && response.body() != null) {
                    serverHashkey = response.body()!!.hashkey
                    binding.tvHashkey.text = "Hashkey: $serverHashkey"
                    binding.tvHashkey.visibility = View.VISIBLE
                    binding.tvStatus.text = "Ready to scan. Click 'Start BLE Scan'"
                    binding.btnStartScan.isEnabled = true
                } else {
                    val errorMsg = response.errorBody()?.string() ?: "Unknown error"
                    binding.tvStatus.text = "Failed to get hashkey: $errorMsg"
                    Toast.makeText(this@BleAuthActivity, "Failed to fetch hashkey", Toast.LENGTH_SHORT).show()
                }

            } catch (e: Exception) {
                binding.tvStatus.text = "Network error: ${e.message}"
                Toast.makeText(this@BleAuthActivity, "Network error", Toast.LENGTH_SHORT).show()
            }
        }
    }

    private fun startBleScan() {
        binding.tvStatus.text = "Scanning for car device..."
        binding.btnStartScan.isEnabled = false

        bleScanner.startScan { deviceAddr, scannedHashkey ->
            runOnUiThread {
                binding.tvDeviceAddress.text = "Device: $deviceAddr"
                binding.tvDeviceAddress.visibility = View.VISIBLE

                // Compare hashkeys
                if (scannedHashkey == serverHashkey) {
                    binding.tvStatus.text = "Hashkey matched! Connecting..."
                    bleScanner.stopScan()
                    deviceAddress = deviceAddr
                    connectToDevice(deviceAddr)
                } else {
                    binding.tvStatus.text = "Hashkey mismatch. Keep scanning..."
                }
            }
        }

        // Auto-stop scan after 30 seconds
        binding.root.postDelayed({
            if (deviceAddress == null) {
                bleScanner.stopScan()
                binding.tvStatus.text = "No device found. Try again."
                binding.btnStartScan.isEnabled = true
            }
        }, 30000)
    }

    private fun connectToDevice(address: String) {
        bleScanner.connect(address) {
            runOnUiThread {
                binding.tvStatus.text = "Connected! Writing ACCESS command..."
                writeAccessCommand()
            }
        }
    }

    private fun writeAccessCommand() {
        bleScanner.writeAccessCommand { success ->
            runOnUiThread {
                if (success) {
                    binding.tvStatus.text = "BLE auth successful! Proceed to NFC..."
                    Toast.makeText(this, "BLE authentication complete", Toast.LENGTH_SHORT).show()

                    // Proceed to NFC authentication
                    startActivity(Intent(this, NfcAuthActivity::class.java))
                    finish()
                } else {
                    binding.tvStatus.text = "Failed to write ACCESS command"
                    Toast.makeText(this, "BLE write failed", Toast.LENGTH_SHORT).show()
                    binding.btnStartScan.isEnabled = true
                }
            }
        }
    }
}
