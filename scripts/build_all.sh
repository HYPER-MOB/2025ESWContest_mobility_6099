#!/usr/bin/env bash
set -e
mkdir -p build logs
LOG=logs/build_$(date +%Y%m%d_%H%M%S).log
echo "[INFO] configure" | tee -a "$LOG"
cmake -S . -B build >> "$LOG" 2>&1
echo "[INFO] build" | tee -a "$LOG"
cmake --build build -j >> "$LOG" 2>&1
echo "[OK] binaries in ./build (또는 CMAKE_RUNTIME_OUTPUT_DIRECTORY가 build로 설정됨)"
