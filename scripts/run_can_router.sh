#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"
LOGROOT="${ROOT}/logs"
LOG_BUILD="${LOGROOT}/build"
LOG_RUN="${LOGROOT}/run"
IFACE="${1:-can0}"

mkdir -p "$BUILD" "$LOG_BUILD" "$LOG_RUN"

ts="$(date +%Y%m%d_%H%M%S)"
bld_log="${LOG_BUILD}/can_router_build_${ts}.log"
run_log="${LOG_RUN}/can_router_run_${ts}.log"

echo "[INFO] CMake configure → $bld_log"
{
  echo "=== CONFIGURE $(date) ==="
  cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_VERBOSE_MAKEFILE=ON "$ROOT"
} 2>&1 | tee "$bld_log"

echo "[INFO] Build target rpi_can_router → $bld_log"
{
  echo
  echo "=== BUILD $(date) ==="
  cmake --build "$BUILD" --target rpi_can_router -- -j
} 2>&1 | tee -a "$bld_log"

echo "[INFO] Run rpi_can_router (${IFACE}) → $run_log"
echo "=== RUN $(date) ===" | tee "$run_log"
"${BUILD}/rpi_can_router" "$IFACE" 2>&1 | tee -a "$run_log"
