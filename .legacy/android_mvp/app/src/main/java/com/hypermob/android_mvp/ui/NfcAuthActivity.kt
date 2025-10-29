package com.hypermob.android_mvp.ui

import android.app.PendingIntent
import android.content.Intent
import android.graphics.Color
import android.nfc.NfcAdapter
import android.nfc.Tag
import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.hypermob.android_mvp.data.api.RetrofitClient
import com.hypermob.android_mvp.data.model.NfcVerifyRequest
import com.hypermob.android_mvp.databinding.ActivityNfcAuthBinding
import com.hypermob.android_mvp.nfc.NfcReader
import com.hypermob.android_mvp.util.PreferenceManager
import kotlinx.coroutines.launch

/**
 * NFC Authentication Activity
 *
 * Final step in MFA authentication:
 * 1. Read NFC tag UID
 * 2. Send to server (POST /auth/nfc/verify)
 * 3. Server validates:
 *    - NFC UID matches registered UID
 *    - Face auth was successful
 *    - BLE auth was successful
 * 4. Show MFA_SUCCESS or MFA_FAILED result
 */
class NfcAuthActivity : AppCompatActivity() {

    private lateinit var binding: ActivityNfcAuthBinding
    private lateinit var prefManager: PreferenceManager
    private var nfcAdapter: NfcAdapter? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityNfcAuthBinding.inflate(layoutInflater)
        setContentView(binding.root)

        prefManager = PreferenceManager(this)
        nfcAdapter = NfcAdapter.getDefaultAdapter(this)

        if (nfcAdapter == null) {
            Toast.makeText(this, "NFC not supported", Toast.LENGTH_SHORT).show()
            finish()
            return
        }

        if (!nfcAdapter!!.isEnabled) {
            Toast.makeText(this, "Please enable NFC", Toast.LENGTH_LONG).show()
        }

        setupUI()
    }

    override fun onResume() {
        super.onResume()
        enableNfcForegroundDispatch()
    }

    override fun onPause() {
        super.onPause()
        disableNfcForegroundDispatch()
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        handleNfcIntent(intent)
    }

    private fun setupUI() {
        binding.btnCancel.setOnClickListener {
            finish()
        }

        binding.btnDone.setOnClickListener {
            // Return to MainActivity
            val intent = Intent(this, MainActivity::class.java)
            intent.flags = Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_NEW_TASK
            startActivity(intent)
            finish()
        }
    }

    private fun enableNfcForegroundDispatch() {
        val intent = Intent(this, javaClass).apply {
            addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP)
        }
        val pendingIntent = PendingIntent.getActivity(
            this, 0, intent,
            PendingIntent.FLAG_MUTABLE
        )

        nfcAdapter?.enableForegroundDispatch(this, pendingIntent, null, null)
    }

    private fun disableNfcForegroundDispatch() {
        nfcAdapter?.disableForegroundDispatch(this)
    }

    private fun handleNfcIntent(intent: Intent?) {
        if (intent?.action == NfcAdapter.ACTION_TAG_DISCOVERED ||
            intent?.action == NfcAdapter.ACTION_TECH_DISCOVERED ||
            intent?.action == NfcAdapter.ACTION_NDEF_DISCOVERED) {

            val tag: Tag? = intent.getParcelableExtra(NfcAdapter.EXTRA_TAG)
            val nfcUid = NfcReader.readUid(tag)

            if (nfcUid != null && NfcReader.isValidUid(nfcUid)) {
                verifyNfcAuth(nfcUid)
            } else {
                Toast.makeText(this, "Invalid NFC tag", Toast.LENGTH_SHORT).show()
            }
        }
    }

    private fun verifyNfcAuth(nfcUid: String) {
        val userId = prefManager.userId
        if (userId == null) {
            Toast.makeText(this, "User not registered", Toast.LENGTH_SHORT).show()
            finish()
            return
        }

        binding.progressBar.visibility = View.VISIBLE
        binding.tvStatus.text = "Verifying NFC authentication..."

        lifecycleScope.launch {
            try {
                val request = NfcVerifyRequest(userId, nfcUid, "CAR123")
                val response = RetrofitClient.authService.verifyNfc(request)

                if (response.isSuccessful && response.body() != null) {
                    val result = response.body()!!

                    if (result.status == "MFA_SUCCESS") {
                        showSuccess(result.message)
                    } else {
                        showFailure(result.message)
                    }
                } else {
                    val errorMsg = response.errorBody()?.string() ?: "Unknown error"
                    showFailure("Verification failed: $errorMsg")
                }

            } catch (e: Exception) {
                showFailure("Network error: ${e.message}")
            } finally {
                binding.progressBar.visibility = View.GONE
            }
        }
    }

    private fun showSuccess(message: String) {
        binding.ivNfc.setColorFilter(Color.GREEN)
        binding.tvStatus.text = "Authentication Successful!"
        binding.tvResult.text = "✓ $message"
        binding.tvResult.setTextColor(Color.GREEN)
        binding.tvResult.visibility = View.VISIBLE
        binding.btnDone.visibility = View.VISIBLE
        binding.btnCancel.visibility = View.GONE

        Toast.makeText(this, "MFA SUCCESS!", Toast.LENGTH_LONG).show()
    }

    private fun showFailure(message: String) {
        binding.ivNfc.setColorFilter(Color.RED)
        binding.tvStatus.text = "Authentication Failed"
        binding.tvResult.text = "✗ $message"
        binding.tvResult.setTextColor(Color.RED)
        binding.tvResult.visibility = View.VISIBLE
        binding.btnDone.visibility = View.VISIBLE
        binding.btnCancel.visibility = View.GONE

        Toast.makeText(this, "MFA FAILED", Toast.LENGTH_LONG).show()
    }
}
