#include "relay_hbridge.hpp"

void RelayHBridge::attach( RelayModule* r, uint8_t a, uint8_t b, uint32_t dead )
{
    relays_  = r;
    chA_     = a;
    chB_     = b;
    dead_ms_ = dead;
    t_       = 0;

    // Safe start
    relays_->set( chA_, false );
    relays_->set( chB_, false );
    state_   = HBState::OFF;
    pending_ = HBState::OFF;
}

// Always: both OFF -> wait dead-time -> apply target state
void RelayHBridge::request( HBState s )
{
    if ( s == state_ ) return;

    // If requesting OFF, apply immediately (no need for dead-time)
    if (s == HBState::OFF) {
        relays_->set( chA_, false );
        relays_->set( chB_, false );
        state_   = HBState::OFF;
        pending_ = HBState::OFF;
        t_       = 0;
        return;
    }

    // For FWD/REV/BRAKE: enforce break-before-make
    relays_->set( chA_, false );
    relays_->set( chB_, false );
    state_   = HBState::OFF;
    pending_ = s;
    t_       = dead_ms_;
}

void RelayHBridge::tick( uint32_t dt_ms )
{
    if ( t_ > 0 ) {
        t_ = ( dt_ms >= t_ ) ? 0 : ( t_ - dt_ms );
        if ( t_ > 0 ) return;

        // Dead-time finished: apply requested state
        switch ( pending_ ) {
            case HBState::FWD:   relays_->set( chA_, true  ); relays_->set( chB_, false ); break;
            case HBState::REV:   relays_->set( chA_, false ); relays_->set( chB_, true  ); break;
            case HBState::BRAKE: relays_->set( chA_, true  ); relays_->set( chB_, true  ); break;
            case HBState::OFF:
            default:             relays_->set( chA_, false ); relays_->set( chB_, false ); break;
        }
        state_   = pending_;
        pending_ = state_;
    }
}
