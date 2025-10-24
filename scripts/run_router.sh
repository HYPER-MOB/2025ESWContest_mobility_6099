#!/usr/bin/env bash
set -e
mkdir -p logs
# can0 up (이미 올렸으면 건너뜀)
if ! ip link show can0 >/dev/null 2>&1; then
  echo "bring up can0 first (mcp2515 사용시 전용 스크립트로)"
fi
LOG=logs/router_run_$(date +%Y%m%d_%H%M%S).log
./rpi_can_router | tee -a "$LOG"
