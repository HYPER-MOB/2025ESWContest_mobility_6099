#pragma once
#include <array>
#include <string>
#include <mutex>
#include <condition_variable>

struct TcuProvided {
    std::array<uint8_t, 8> expected_nfc{};
    bool has_nfc = false;

    std::array<uint8_t, 8> ble_hash{};
    bool has_ble_hash = false;

    std::mutex m;
    std::condition_variable cv;

    static std::string to_hex16(const std::array<uint8_t, 8>& b) {
        static const char* HEX = "0123456789ABCDEF";
        std::string s; s.resize(16);
        for (int i = 0; i < 8; i++) { s[i * 2] = HEX[(b[i] >> 4) & 0xF]; s[i * 2 + 1] = HEX[b[i] & 0xF]; }
        return s;
    }
    static std::string to_hex_uid(const std::array<uint8_t, 8>& b) {
        return to_hex16(b);
    }
};
