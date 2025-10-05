#pragma once
#include <cstdint>
#include "relay_module.hpp"

/** H-bridge states for a DC actuator driven by two relays (A/B). */
enum class HBState : uint8_t { OFF=0, FWD=1, REV=2, BRAKE=3 };

/**
 * RelayHBridge
 * -------------------------------------------------------------------------
 * Wraps two relay channels (A/B) into a safe H-bridge:
 * - Any transition goes through OFF with break-before-make dead-time.
 * - Call request() to ask for new state; call tick(dt) periodically to run it.
 */
class RelayHBridge
{
public:
    /** Attach to RelayModule and set channel indices + dead-time (ms). */
    void attach( RelayModule* relays, uint8_t chA, uint8_t chB, uint32_t dead_ms );

    /** Non-blocking request. Immediately forces OFF and starts dead-time. */
    void request( HBState s );

    /** Periodic runner. Call with CTRL_TICK_MS. */
    void tick( uint32_t dt_ms );

    HBState state() const { return state_; }

private:
    RelayModule* relays_ { nullptr };
    uint8_t      chA_    { 0 };
    uint8_t      chB_    { 1 };
    uint32_t     dead_ms_{ 80 };
    uint32_t     t_      { 0 };
    HBState      state_  { HBState::OFF };
    HBState      pending_{ HBState::OFF };
};
