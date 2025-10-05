#pragma once
#include <cstdint>
#include <functional>

/** Minimal CAN message wrapper for convenience. */
struct CanMsg {
  uint16_t id;
  uint8_t  data[8];
  uint8_t  dlc{8};
};

/**
 * CanNode — thin wrapper over ESP-IDF TWAI driver.
 * - begin(): init/start CAN (TWAI)
 * - pollRx(): non-blocking receive poll; dispatches to onFrame()
 * - sendState(): transmit 8-byte frame
 * - onSeatCmd: callback for Seat command ID-range (set by SeatNode)
 */
class CanNode {
public:
  bool begin();
  void pollRx();
  bool sendState(uint16_t id, const uint8_t data[8]);

  std::function<void(const CanMsg&)> onSeatCmd;

  void onFrame(uint16_t id, const uint8_t d[8]);
};
