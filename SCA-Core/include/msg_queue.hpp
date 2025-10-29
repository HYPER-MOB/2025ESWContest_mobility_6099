#pragma once
#include <condition_variable>
#include <mutex>
#include <queue>

template<typename T>
class MsgQueue {
public:
    void push(const T& v) { { std::lock_guard<std::mutex> lk(m_); q_.push(v); } cv_.notify_one(); }
    bool pop(T& out) {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [&] { return stop_ || !q_.empty(); });
        if (q_.empty()) return false;
        out = q_.front(); q_.pop(); return true;
    }
    void shutdown() { { std::lock_guard<std::mutex> lk(m_); stop_ = true; } cv_.notify_all(); }
private:
    std::queue<T> q_;
    std::mutex m_;
    std::condition_variable cv_;
    bool stop_ = false;
};
