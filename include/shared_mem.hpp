#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <vector>

struct CanSlot {
    uint32_t id = 0;
    uint8_t  dlc = 0;
    std::array<uint8_t, 8> data{};
    std::atomic<bool> dirty{ false };
    int period_ms = 0;
    std::chrono::steady_clock::time_point next_due{};
};

struct SharedMem {
    std::vector<CanSlot> slots;
    std::mutex m;
    CanSlot& ensure_slot(uint32_t id, int period_ms) {
        std::lock_guard<std::mutex> lk(m);
        for (auto& s : slots) if (s.id == id) return s;
        slots.push_back(CanSlot{});
        auto& s = slots.back();
        s.id = id; s.dlc = 0; s.period_ms = period_ms;
        s.next_due = std::chrono::steady_clock::now();
        return s;
    }
};
