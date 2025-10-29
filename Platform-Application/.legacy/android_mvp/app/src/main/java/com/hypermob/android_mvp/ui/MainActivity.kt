package com.hypermob.android_mvp.ui

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.hypermob.android_mvp.databinding.ActivityMainBinding
import com.hypermob.android_mvp.util.PreferenceManager

/**
 * Main Activity - Entry point for HYPERMOB MFA App
 *
 * State Machine:
 * 1. NOT_REGISTERED -> Face Registration
 * 2. FACE_REGISTERED -> NFC Registration
 * 3. FULLY_REGISTERED -> Authentication Ready
 */
class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var prefManager: PreferenceManager

    private val requiredPermissions = mutableListOf(
        Manifest.permission.CAMERA,
        Manifest.permission.NFC,
        Manifest.permission.ACCESS_FINE_LOCATION
    ).apply {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            add(Manifest.permission.BLUETOOTH_SCAN)
            add(Manifest.permission.BLUETOOTH_CONNECT)
        } else {
            add(Manifest.permission.BLUETOOTH)
            add(Manifest.permission.BLUETOOTH_ADMIN)
        }
    }

    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { permissions ->
        val allGranted = permissions.values.all { it }
        if (!allGranted) {
            Toast.makeText(this, "All permissions are required", Toast.LENGTH_LONG).show()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        prefManager = PreferenceManager(this)

        requestPermissions()
        setupUI()
        updateUI()
    }

    override fun onResume() {
        super.onResume()
        updateUI()
    }

    private fun requestPermissions() {
        val permissionsToRequest = requiredPermissions.filter {
            ContextCompat.checkSelfPermission(this, it) != PackageManager.PERMISSION_GRANTED
        }

        if (permissionsToRequest.isNotEmpty()) {
            permissionLauncher.launch(permissionsToRequest.toTypedArray())
        }
    }

    private fun setupUI() {
        binding.btnRegisterFace.setOnClickListener {
            startActivity(Intent(this, FaceRegisterActivity::class.java))
        }

        binding.btnStartAuth.setOnClickListener {
            // Start unified authentication flow (BLE + NFC)
            startActivity(Intent(this, AuthenticationActivity::class.java))
        }

        binding.btnDebugNfcReader.setOnClickListener {
            // Debug: Read HCE UID from another phone
            startActivity(Intent(this, NfcDebugReaderActivity::class.java))
        }

        binding.btnClearData.setOnClickListener {
            prefManager.clear()
            Toast.makeText(this, "All data cleared", Toast.LENGTH_SHORT).show()
            updateUI()
        }
    }

    private fun updateUI() {
        val userId = prefManager.userId
        val faceId = prefManager.faceId
        val nfcUid = prefManager.nfcUid

        // Update user info display
        if (userId != null) {
            binding.layoutUserInfo.visibility = View.VISIBLE
            binding.tvUserId.text = "User ID: $userId"
            binding.tvFaceId.text = "Face ID: ${faceId ?: "Not registered"}"
            binding.tvNfcUid.text = "NFC UID: ${nfcUid ?: "Not registered"}"
        } else {
            binding.layoutUserInfo.visibility = View.GONE
        }

        // Simplified state machine (no NFC registration needed)
        when {
            userId == null || nfcUid == null -> {
                // State: NOT_REGISTERED
                binding.tvStatus.text = "Please register"
                binding.btnRegisterFace.isEnabled = true
                binding.btnStartAuth.isEnabled = false
            }
            else -> {
                // State: REGISTERED - Ready for authentication
                binding.tvStatus.text = "Ready for authentication"
                binding.btnRegisterFace.isEnabled = true
                binding.btnStartAuth.isEnabled = true
            }
        }
    }
}
