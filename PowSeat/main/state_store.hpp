#pragma once
#include <cstdint>
#include <cstring>
#include "nvs_flash.h"

/**
 * Persisted data schema (NVS)
 * - positions: per-axis open-loop position [ticks]
 * - speed_pct: per-axis speed setpoint [%] (reserved for future PWM driver)
 */
struct PersistData {
  int16_t pos[4];         // AXIS_COUNT assumed 4 here
  uint8_t speed_pct[4];   // 0..100 (%), not used by relays yet
};

class StateStore {
public:
  bool begin();                   // init NVS + open namespace
  bool load(PersistData& out);    // returns false if not found
  bool save(const PersistData& in);

private:
  nvs_handle_t h_ { 0 };
};
