#pragma once
#include "can_node.hpp"
#include "relay_module.hpp"
#include "relay_hbridge.hpp"
#include "config.h"
#include "utils.hpp"

/**
 * SeatNode
 * -------------------------------------------------------------------------
 * High-level coordinator for 4 axes using 8 relays (two per axis).
 * - Consumes CAN commands
 * - Drives RelayHBridge per axis (with dead-time safety)
 * - (Optional) maintains a simple open-loop position target for demo/testing
 */
class SeatNode {
public:
  bool begin(CanNode* can);
  void onCanCmd(const CanMsg& m);
  void tick10ms();
  void report20Hz();

private:
  void axisDrive(Axis ax, HBState s);

  CanNode*    can_{nullptr};
  RelayModule relays_;
  RelayHBridge hb_[AXIS_COUNT];

  // Optional: crude open-loop 'position' (ticks) for each axis
  int pos_[AXIS_COUNT]{0,0,0,0};
  int target_[AXIS_COUNT]{0,0,0,0};

  // Persistence
  StateStore store_;
  PersistData persist_{};

  // Initial homing
  bool homing_active_{false};
  uint32_t homing_elapsed_ms_{0};
};
