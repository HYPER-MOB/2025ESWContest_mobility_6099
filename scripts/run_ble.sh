#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"
LOGROOT="${ROOT}/logs"
LOG_BUILD="${LOGROOT}/build"
LOG_RUN="${LOGROOT}/run"
DUR="${1:-5}"

mkdir -p "$BUILD" "$LOG_BUILD" "$LOG_RUN"

ts="$(date +%Y%m%d_%H%M%S)"
bld_log="${LOG_BUILD}/ble_build_${ts}.log"
run_log="${LOG_RUN}/ble_run_${ts}.log"

echo "[INFO] CMake configure → $bld_log"
{
  echo "=== CONFIGURE $(date) ==="
  cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_VERBOSE_MAKEFILE=ON "$ROOT"
} 2>&1 | tee "$bld_log"

echo "[INFO] Build target test_ble_log → $bld_log"
{
  echo
  echo "=== BUILD $(date) ==="
  cmake --build "$BUILD" --target test_ble_log -- -j
} 2>&1 | tee -a "$bld_log"

echo "[INFO] Run test_ble_log (${DUR}s) → $run_log"
echo "=== RUN $(date) ===" | tee "$run_log"
"${BUILD}/test_ble_log" "$DUR" 2>&1 | tee -a "$run_log"
