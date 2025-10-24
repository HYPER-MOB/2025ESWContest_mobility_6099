#include "can_bus.hpp"
#include "can_ids.hpp"
#include "config/app_config.h"
#include "msg_queue.hpp"
#include "sequencer.hpp"
#include "shared_mem.hpp"
#include "tcu_store.hpp"

#include <atomic>
#include <iostream>
#include <thread>

void mem_writer_loop(std::atomic<bool>& running, SharedMem& shm, const SeqContext& ctx);
void tx_loop(std::atomic<bool>& running, SharedMem& shm, CANBus& bus);
struct RxItem { uint32_t id; uint8_t dlc; uint8_t data[8]; };
void rx_loop(std::atomic<bool>& running, CANBus& bus, MsgQueue<RxItem>& q, TcuProvided& tcu);
void seq_loop(std::atomic<bool>& running, MsgQueue<RxItem>& q, SharedMem& shm, SeqContext& ctx, CANBus& bus, TcuProvided& tcu);

int main() {
    CANBus bus;
    if (!bus.open(CAN_IFACE)) {
        std::cerr << "[ERR] CAN open failed on " << CAN_IFACE << "\n";
        return 1;
    }

    std::atomic<bool> running{ true };
    SharedMem shm;
    MsgQueue<RxItem> rxq;
    SeqContext ctx;
    TcuProvided tcu;

    // 상태 주기 슬롯
    shm.ensure_slot(CANID::SCA_DCU_AUTH_STATE, AUTH_STATE_PERIOD_MS);

    std::thread th_writer(mem_writer_loop, std::ref(running), std::ref(shm), std::cref(ctx));
    std::thread th_tx(tx_loop, std::ref(running), std::ref(shm), std::ref(bus));
    std::thread th_rx(rx_loop, std::ref(running), std::ref(bus), std::ref(rxq), std::ref(tcu));
    std::thread th_seq(seq_loop, std::ref(running), std::ref(rxq), std::ref(shm), std::ref(ctx), std::ref(bus), std::ref(tcu));

    std::cout << "[router] ENTER 키를 누르면 종료합니다.\n";
    std::string dummy; std::getline(std::cin, dummy);

    running.store(false);
    rxq.shutdown();

    if (th_seq.joinable())    th_seq.join();
    if (th_rx.joinable())     th_rx.join();
    if (th_tx.joinable())     th_tx.join();
    if (th_writer.joinable()) th_writer.join();

    std::cout << "[router] bye\n";
    return 0;
}
