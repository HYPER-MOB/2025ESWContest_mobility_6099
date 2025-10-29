package com.hypermob.android_mvp.ui

import android.app.PendingIntent
import android.content.Intent
import android.nfc.NfcAdapter
import android.nfc.Tag
import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.hypermob.android_mvp.data.api.RetrofitClient
import com.hypermob.android_mvp.data.model.NfcRegisterRequest
import com.hypermob.android_mvp.databinding.ActivityNfcRegisterBinding
import com.hypermob.android_mvp.nfc.NfcReader
import com.hypermob.android_mvp.util.PreferenceManager
import kotlinx.coroutines.launch

/**
 * NFC Registration Activity
 * - Reads NFC tag UID
 * - Uploads to server via POST /auth/nfc
 * - Saves nfc_uid to SharedPreferences
 */
class NfcRegisterActivity : AppCompatActivity() {

    private lateinit var binding: ActivityNfcRegisterBinding
    private lateinit var prefManager: PreferenceManager
    private var nfcAdapter: NfcAdapter? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityNfcRegisterBinding.inflate(layoutInflater)
        setContentView(binding.root)

        prefManager = PreferenceManager(this)
        nfcAdapter = NfcAdapter.getDefaultAdapter(this)

        if (nfcAdapter == null) {
            Toast.makeText(this, "NFC not supported on this device", Toast.LENGTH_LONG).show()
            finish()
            return
        }

        if (!nfcAdapter!!.isEnabled) {
            Toast.makeText(this, "Please enable NFC in Settings", Toast.LENGTH_LONG).show()
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
                binding.tvNfcUid.text = "UID: $nfcUid"
                binding.tvNfcUid.visibility = View.VISIBLE
                uploadNfcUid(nfcUid)
            } else {
                Toast.makeText(this, "Invalid NFC tag", Toast.LENGTH_SHORT).show()
            }
        }
    }

    private fun uploadNfcUid(nfcUid: String) {
        val userId = prefManager.userId
        if (userId == null) {
            Toast.makeText(this, "Please register face first", Toast.LENGTH_SHORT).show()
            finish()
            return
        }

        binding.progressBar.visibility = View.VISIBLE
        binding.tvStatus.text = "Registering NFC..."

        lifecycleScope.launch {
            try {
                val request = NfcRegisterRequest(userId, nfcUid)
                val response = RetrofitClient.authService.registerNfc(request)

                if (response.isSuccessful && response.body() != null) {
                    // Save to preferences
                    prefManager.nfcUid = nfcUid

                    binding.tvStatus.text = "NFC registered successfully!"
                    Toast.makeText(this@NfcRegisterActivity, "NFC registration complete", Toast.LENGTH_LONG).show()

                    // Return to MainActivity
                    finish()
                } else {
                    val errorMsg = response.errorBody()?.string() ?: "Unknown error"
                    binding.tvStatus.text = "Failed: $errorMsg"
                    Toast.makeText(this@NfcRegisterActivity, "Registration failed", Toast.LENGTH_SHORT).show()
                }

            } catch (e: Exception) {
                binding.tvStatus.text = "Error: ${e.message}"
                Toast.makeText(this@NfcRegisterActivity, "Network error", Toast.LENGTH_SHORT).show()
            } finally {
                binding.progressBar.visibility = View.GONE
            }
        }
    }
}
