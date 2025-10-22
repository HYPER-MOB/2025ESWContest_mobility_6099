#include <nfc/nfc.h>
#include <cstdio>
#include <cstring>
#include <string>

static std::string now() {
    char buf[64]; time_t t = time(nullptr);
    strftime(buf, sizeof(buf), "%F %T", localtime(&t)); return buf;
}

int main() {
    const char* LOG = "logs/nfc_run.log";
    FILE* lf = fopen(LOG, "a");
    if (!lf) { perror("open log"); return 1; }
    fprintf(lf, "[%s] === NFC TEST START ===\n", now().c_str()); fflush(lf);

    nfc_context* ctx = nullptr;
    nfc_init(&ctx);
    if (!ctx) { fprintf(lf, "nfc_init failed\n"); fclose(lf); return 2; }

    nfc_device* pnd = nfc_open(ctx, nullptr); // 첫 장치
    if (!pnd) { fprintf(lf, "no nfc device\n"); nfc_exit(ctx); fclose(lf); return 3; }
    if (nfc_initiator_init(pnd) < 0) { fprintf(lf, "init initiator fail\n"); nfc_close(pnd); nfc_exit(ctx); fclose(lf); return 4; }

    // ISO14443A 한 번 폴링
    nfc_target nt;
    const nfc_modulation nm = { NMT_ISO14443A, NBR_106 };
    int rc = nfc_initiator_poll_target(pnd, &nm, 1, 2, 150, &nt);

    if (rc > 0) {
        fprintf(lf, "[%s] NFC tag detected.\n", now().c_str());
        if (nt.nti.nai.szUidLen > 0) {
            fprintf(lf, "UID: ");
            for (size_t i = 0; i < nt.nti.nai.szUidLen; i++) fprintf(lf, "%02X", nt.nti.nai.abtUid[i]);
            fprintf(lf, "\n");
        }
    }
    else {
        fprintf(lf, "[%s] no tag\n", now().c_str());
    }

    nfc_close(pnd); nfc_exit(ctx);
    fprintf(lf, "[%s] === NFC TEST END ===\n\n", now().c_str());
    fclose(lf);
    return 0;
}
