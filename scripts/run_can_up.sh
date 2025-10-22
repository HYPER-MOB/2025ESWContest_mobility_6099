#!/usr/bin/env bash
set -euo pipefail

IFACE="${1:-can0}"
BITRATE="${2:-500000}"        # 기본 500k
RESTART_MS="${3:-100}"        # 버스오프 자동복구(ms), 원치 않으면 0
DBitrate=""                   # 클래식 CAN이면 비워둠 (CAN FD 쓸 때만)

echo "[INFO] Load kernel modules"
sudo modprobe can
sudo modprobe can_raw
sudo modprobe mcp251x        # MCP2515 드라이버
sudo modprobe spi_bcm2835    # SPI 드라이버

# 기존에 up이면 내려주기
if ip link show "$IFACE" &>/dev/null; then
  sudo ip link set "$IFACE" down || true
fi

echo "[INFO] Bring up $IFACE bitrate=${BITRATE}, restart-ms=${RESTART_MS}"
# 클래식 CAN
sudo ip link set "$IFACE" type can bitrate "$BITRATE" restart-ms "$RESTART_MS"
sudo ip link set "$IFACE" up

echo "[OK] $IFACE is up"
ip -details link show "$IFACE" | sed 's/^\s\+//'