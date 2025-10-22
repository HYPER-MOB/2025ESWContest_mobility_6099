#include <nfc/nfc.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <ctime>

static std::string now() {
    char buf[64]; std::time_t t = std::time(nullptr);
    std::strftime(buf, sizeof(buf), "%F %T", std::localtime(&t));
    return buf;
}

int main() {
    const char* LOG = "logs/nfc_run.log";
    FILE* lf = std::fopen(LOG, "a");
    if (!lf) { perror("open log"); return 1; }
    std::fprintf(lf, "[%s] === NFC TEST START ===\n", now().c_str()); std::fflush(lf);

    nfc_context* ctx = nullptr; nfc_init(&ctx);
    if (!ctx) { std::fprintf(lf, "nfc_init failed\n"); std::fclose(lf); return 2; }

    nfc_device* pnd = nfc_open(ctx, nullptr);
    if (!pnd) { std::fprintf(lf, "no nfc device\n"); nfc_exit(ctx); std::fclose(lf); return 3; }
    if (nfc_initiator_init(pnd) < 0) {
        std::fprintf(lf, "init initiator fail\n");
        nfc_close(pnd); nfc_exit(ctx); std::fclose(lf); return 4;
    }

    nfc_target nt;
    const nfc_modulation nm = { NMT_ISO14443A, NBR_106 };
    int rc = nfc_initiator_poll_target(pnd, &nm, 1, 2, 150, &nt);

    if (rc > 0) {
        std::fprintf(lf, "[%s] NFC tag detected.\n", now().c_str());
        if (nt.nti.nai.szUidLen > 0) {
            std::fprintf(lf, "UID: ");
            for (size_t i = 0; i < nt.nti.nai.szUidLen; i++) std::fprintf(lf, "%02X", nt.nti.nai.abtUid[i]);
            std::fprintf(lf, "\n");
        }
    }
    else {
        std::fprintf(lf, "[%s] no tag\n", now().c_str());
    }

    nfc_close(pnd); nfc_exit(ctx);
    std::fprintf(lf, "[%s] === NFC TEST END ===\n\n", now().c_str());
    std::fclose(lf);
    return 0;
}
