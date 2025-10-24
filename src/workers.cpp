#include "can_bus.hpp"
#include "can_ids.hpp"
#include "config/app_config.h"
#include "msg_queue.hpp"
#include "sequencer.hpp"
#include "shared_mem.hpp"
#include "tcu_store.hpp"

#include <sys/epoll.h>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std::chrono;

static inline void logln(const char* tag, const std::string& msg) {
    auto t = system_clock::now();
    auto tt = system_clock::to_time_t(t);
    auto ms = duration_cast<milliseconds>(t.time_since_epoch()) % 1000;
    std::tm tm{}; localtime_r(&tt, &tm);
    std::cerr << "[" << std::put_time(&tm, "%F %T") << "."
        << std::setw(3) << std::setfill('0') << ms.count()
        << "][" << tag << "] " << msg << "\n";
}

struct RxItem { uint32_t id; uint8_t dlc; uint8_t data[8]; };

void mem_writer_loop(std::atomic<bool>& running, SharedMem& shm, const SeqContext& ctx) {
    auto next = steady_clock::now();
    while (running.load()) {
        auto& s = shm.ensure_slot(CANID::SCA_DCU_AUTH_STATE, AUTH_STATE_PERIOD_MS);
        s.dlc = 2;
        s.data[0] = static_cast<uint8_t>(ctx.step.load());
        s.data[1] = static_cast<uint8_t>(ctx.flag.load());
        std::this_thread::sleep_until(next += milliseconds(AUTH_STATE_PERIOD_MS));
    }
}

void tx_loop(std::atomic<bool>& running, SharedMem& shm, CANBus& bus) {
    while (running.load()) {
        auto now = steady_clock::now();
        {
            std::lock_guard<std::mutex> lk(shm.m);
            for (auto& s : shm.slots) {
                bool due = (s.period_ms > 0 && now >= s.next_due);
                if (s.dirty.load() || due) {
                    (void)bus.send(s.id, s.data.data(), s.dlc);
                    s.dirty = false;
                    if (s.period_ms > 0) s.next_due = now + milliseconds(s.period_ms);
                }
            }
        }
        std::this_thread::sleep_for(milliseconds(TX_TICK_MS));
    }
}

void rx_loop(std::atomic<bool>& running, CANBus& bus, MsgQueue<RxItem>& q, TcuProvided& tcu) {
    int ep = epoll_create1(0);
    epoll_event ev{}; ev.events = EPOLLIN; ev.data.fd = bus.fd;
    epoll_ctl(ep, EPOLL_CTL_ADD, bus.fd, &ev);

    while (running.load()) {
        epoll_event out{};
        int r = epoll_wait(ep, &out, 1, 50);
        if (r <= 0) continue;
        if (out.events & EPOLLIN) {
            struct can_frame f {};
            if (!bus.recv(f)) continue;

            if (f.can_id == CANID::TCU_SCA_USER_INFO_NFC && f.can_dlc >= 8) {
                std::lock_guard<std::mutex> lk(tcu.m);
                for (int i = 0; i < 8; i++) tcu.expected_nfc[i] = f.data[i];
                tcu.has_nfc = true; tcu.cv.notify_all();
                logln("RX", "0x107 기대 NFC UID 수신");
            }
            else if (f.can_id == CANID::TCU_SCA_USER_INFO_BLE_SESS && f.can_dlc >= 8) {
                std::lock_guard<std::mutex> lk(tcu.m);
                for (int i = 0; i < 8; i++) tcu.ble_hash[i] = f.data[i];
                tcu.has_ble_hash = true; tcu.cv.notify_all();
                logln("RX", "0x108 BLE 해시(8B) 수신");
            }
            else {
                RxItem it{ f.can_id, f.can_dlc, {0} };
                if (f.can_dlc) std::memcpy(it.data, f.data, f.can_dlc);
                q.push(it);
            }
        }
    }
    close(ep);
}

void seq_loop(std::atomic<bool>& running, MsgQueue<RxItem>& q, SharedMem& shm,
    SeqContext& ctx, CANBus&, TcuProvided& tcu)
{
    while (running.load()) {
        RxItem r{};
        if (!q.pop(r)) continue;

        if (r.id == CANID::DCU_SCA_USER_FACE_REQ) {
            logln("SEQ", "트리거(0x101) → NFC→BLE→Camera");

            // NFC 준비 대기(최대 3초)
            {
                std::unique_lock<std::mutex> lk(tcu.m);
                if (!tcu.has_nfc)
                    tcu.cv.wait_for(lk, std::chrono::seconds(3), [&] { return tcu.has_nfc; });
            }
            bool nfc_ok = tcu.has_nfc && run_nfc(ctx, tcu);

            // NFC ACK (index=1)
            {
                auto& s = shm.ensure_slot(CANID::SCA_TCU_USER_INFO_ACK, 0);
                s.dlc = 2; s.data[0] = 1; s.data[1] = nfc_ok ? 0x00 : 0x01; s.dirty = true;
            }
            if (!nfc_ok) {
                auto& res = shm.ensure_slot(CANID::SCA_DCU_AUTH_RESULT, 0);
                res.dlc = 8; res.data[0] = 0x01;
                const char* id = "NFCFAIL"; for (int i = 0; i < 7; i++) res.data[1 + i] = (uint8_t)id[i];
                res.dirty = true; ctx.step = AuthStep::Error; ctx.flag = AuthFlag::FAIL;
                continue;
            }

            // BLE 준비 대기(최대 3초)
            {
                std::unique_lock<std::mutex> lk(tcu.m);
                if (!tcu.has_ble_hash)
                    tcu.cv.wait_for(lk, std::chrono::seconds(3), [&] { return tcu.has_ble_hash; });
            }
            bool ble_ok = tcu.has_ble_hash && run_ble(ctx, tcu);
            if (!ble_ok) {
                auto& res = shm.ensure_slot(CANID::SCA_DCU_AUTH_RESULT, 0);
                res.dlc = 8; res.data[0] = 0x01;
                const char* id = "BLEFAIL"; for (int i = 0; i < 7; i++) res.data[1 + i] = (uint8_t)id[i];
                res.dirty = true; ctx.step = AuthStep::Error; ctx.flag = AuthFlag::FAIL;
                continue;
            }

            // Camera
            bool cam_ok = run_camera(ctx);

            // 최종 결과(성공 시 USER_ID 사용, 없으면 기본)
            {
                auto& res = shm.ensure_slot(CANID::SCA_DCU_AUTH_RESULT, 0);
                res.dlc = 8;
                res.data[0] = (nfc_ok && ble_ok && cam_ok) ? 0x00 : 0x01;
                const std::string uid = (!ctx.user_id.empty() ? ctx.user_id : "USR0001");
                for (int i = 0; i < 7; i++) res.data[1 + i] = (i < (int)uid.size()) ? (uint8_t)uid[i] : (uint8_t)0;
                res.dirty = true;
            }
        }
    }
}
