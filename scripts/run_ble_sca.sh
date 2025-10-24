#!/usr/bin/env bash
set -euo pipefail

# scripts/run_ble_sca
# - CMake out-of-source configure & build
# - No run: you will run sca_ble_peripheral yourself with sudo and args.

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"
LOGROOT="${ROOT}/logs"
LOG_BUILD="${LOGROOT}/build"

mkdir -p "$BUILD" "$LOG_BUILD"

ts="$(date +%Y%m%d_%H%M%S)"
bld_log="${LOG_BUILD}/sca_ble_peripheral_${ts}.log"

echo "[INFO] CMake configure(out-of-source) ¡æ $bld_log"
{
  echo "=== CONFIGURE $(date) ==="
  cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_VERBOSE_MAKEFILE=ON
} 2>&1 | tee "$bld_log"

echo "[INFO] Build target sca_ble_peripheral ¡æ $bld_log"
{
  echo
  echo "=== BUILD $(date) ==="
  cmake --build "$BUILD" --target sca_ble_peripheral -j
} 2>&1 | tee -a "$bld_log"

BIN="${BUILD}/sca_ble_peripheral"
if [[ -x "$BIN" ]]; then
  echo "[OK] Built: $BIN"
  echo
  echo "Run it yourself (example):"
  echo "  sudo \"$BIN\" --hash A1B2C3D4E5F60708 --name CAR123 --timeout 30 --token ACCESS --require-encrypt 0"
else
  echo "[ERR] binary not found or not executable: $BIN" | tee -a "$bld_log"
  exit 2
fi
