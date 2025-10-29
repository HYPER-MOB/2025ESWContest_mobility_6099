package com.hypermob.android_mvp.ui

import android.app.PendingIntent
import android.content.Intent
import android.nfc.NfcAdapter
import android.nfc.Tag
import android.nfc.tech.IsoDep
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.hypermob.android_mvp.databinding.ActivityNfcDebugReaderBinding

/**
 * NFC Debug Reader Activity
 *
 * 다른 폰의 HCE UID를 읽어서 확인하는 디버그 기능
 */
class NfcDebugReaderActivity : AppCompatActivity() {

    private lateinit var binding: ActivityNfcDebugReaderBinding
    private var nfcAdapter: NfcAdapter? = null

    companion object {
        private const val TAG = "NfcDebugReader"

        // HYPERMOB HCE 앱의 AID
        private val AID = byteArrayOf(
            0xF0.toByte(), 0x01, 0x02, 0x03, 0x04, 0x05, 0x06
        )
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityNfcDebugReaderBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setupNfc()
        setupUI()
    }

    private fun setupNfc() {
        nfcAdapter = NfcAdapter.getDefaultAdapter(this)

        if (nfcAdapter == null) {
            binding.tvStatus.text = "❌ NFC not supported on this device"
            Toast.makeText(this, "NFC not supported", Toast.LENGTH_LONG).show()
            return
        }

        if (!nfcAdapter!!.isEnabled) {
            binding.tvStatus.text = "⚠️ NFC is disabled\n\nPlease enable NFC in settings"
            Toast.makeText(this, "Please enable NFC", Toast.LENGTH_LONG).show()
        } else {
            binding.tvStatus.text = "✓ NFC Reader Ready\n\nTap another phone with HYPERMOB app running"
        }
    }

    private fun setupUI() {
        binding.btnBack.setOnClickListener {
            finish()
        }

        binding.btnClearResult.setOnClickListener {
            binding.tvResult.text = "No result yet"
            binding.tvResult.visibility = View.GONE
        }
    }

    override fun onResume() {
        super.onResume()

        nfcAdapter?.let { adapter ->
            val pendingIntent = PendingIntent.getActivity(
                this, 0,
                Intent(this, javaClass).addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP),
                PendingIntent.FLAG_MUTABLE
            )

            adapter.enableForegroundDispatch(this, pendingIntent, null, null)
            Log.d(TAG, "NFC foreground dispatch enabled")
        }
    }

    override fun onPause() {
        super.onPause()
        nfcAdapter?.disableForegroundDispatch(this)
        Log.d(TAG, "NFC foreground dispatch disabled")
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)

        if (NfcAdapter.ACTION_TAG_DISCOVERED == intent.action ||
            NfcAdapter.ACTION_TECH_DISCOVERED == intent.action) {

            val tag = intent.getParcelableExtra<Tag>(NfcAdapter.EXTRA_TAG)
            if (tag != null) {
                readHceUid(tag)
            }
        }
    }

    private fun readHceUid(tag: Tag) {
        binding.progressBar.visibility = View.VISIBLE
        binding.tvStatus.text = "Reading..."

        val isoDep = IsoDep.get(tag)
        if (isoDep == null) {
            showError("Not an ISO-DEP tag")
            return
        }

        try {
            isoDep.connect()
            isoDep.timeout = 5000

            val hardwareUid = tag.id.toHexString()
            Log.d(TAG, "Hardware UID: $hardwareUid")

            updateStatus("Connected!\n\nHardware UID: $hardwareUid\n\nSending SELECT AID...")

            // SELECT AID APDU
            val selectApdu = byteArrayOf(
                0x00,           // CLA
                0xA4.toByte(),  // INS (SELECT)
                0x04,           // P1
                0x00,           // P2
                AID.size.toByte() // Lc
            ) + AID

            Log.d(TAG, "→ SELECT AID: ${selectApdu.toHexString()}")

            // Send APDU
            val response = isoDep.transceive(selectApdu)

            Log.d(TAG, "← Response (${response.size} bytes): ${response.toHexString()}")

            // Parse response
            if (response.size < 2) {
                showError("Invalid response length: ${response.size}")
                return
            }

            val sw1 = response[response.size - 2].toInt() and 0xFF
            val sw2 = response[response.size - 1].toInt() and 0xFF
            val statusCode = "${sw1.toString(16).uppercase().padStart(2, '0')}${sw2.toString(16).uppercase().padStart(2, '0')}"

            if (sw1 == 0x90 && sw2 == 0x00) {
                // Success!
                val hceUid = response.copyOfRange(0, response.size - 2)
                val hceUidHex = hceUid.toHexString()

                val result = """
                    ✅ SUCCESS!

                    Hardware UID (${tag.id.size} bytes):
                    $hardwareUid

                    HCE UID (${hceUid.size} bytes):
                    $hceUidHex

                    Status: $statusCode

                    Full Response:
                    ${response.toHexString()}
                """.trimIndent()

                showSuccess(result)

                Log.d(TAG, "✓ HCE UID: $hceUidHex")

            } else {
                showError("APDU Failed\n\nStatus: $statusCode\n\nResponse:\n${response.toHexString()}")
            }

            isoDep.close()

        } catch (e: Exception) {
            Log.e(TAG, "Error reading HCE UID", e)
            showError("Error: ${e.message}\n\n${e.javaClass.simpleName}")
        }
    }

    private fun updateStatus(text: String) {
        runOnUiThread {
            binding.tvStatus.text = text
        }
    }

    private fun showSuccess(result: String) {
        runOnUiThread {
            binding.progressBar.visibility = View.GONE
            binding.tvStatus.text = "✓ Read Complete!"
            binding.tvResult.text = result
            binding.tvResult.visibility = View.VISIBLE
            Toast.makeText(this, "HCE UID read successfully!", Toast.LENGTH_SHORT).show()
        }
    }

    private fun showError(message: String) {
        runOnUiThread {
            binding.progressBar.visibility = View.GONE
            binding.tvStatus.text = "❌ Error"
            binding.tvResult.text = message
            binding.tvResult.visibility = View.VISIBLE
            Toast.makeText(this, "Failed to read HCE UID", Toast.LENGTH_SHORT).show()
        }
    }

    private fun ByteArray.toHexString(): String {
        return joinToString("") { "%02X".format(it) }
    }
}
