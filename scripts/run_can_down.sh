#!/usr/bin/env bash
set -euo pipefail
IFACE="${1:-can0}"
if ip link show "$IFACE" &>/dev/null; then
  sudo ip link set "$IFACE" down || true
  echo "[OK] $IFACE is down"
else
  echo "[INFO] $IFACE not found"
fi
