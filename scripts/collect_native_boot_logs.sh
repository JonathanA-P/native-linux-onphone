#!/usr/bin/env bash
# scripts/collect_native_boot_logs.sh — Collect native boot hardware logs from diagnostic console
set -e

WORKSPACE_DIR="/home/jonathan/Documents/Projects/Apps/a70/native-linux"
LOGS_DIR="$WORKSPACE_DIR/logs/native-boot"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TARGET_LOG_DIR="$LOGS_DIR/boot_$TIMESTAMP"
DEVICE_IP="192.168.42.2"
SSH_KEY="$HOME/.ssh/id_ed25519"

mkdir -p "$TARGET_LOG_DIR"

echo "================================================================="
echo "  SAMSUNG GALAXY A70 — NATIVE BOOT LOG COLLECTOR"
echo "  Target Host: root@$DEVICE_IP:22"
echo "  Destination: $TARGET_LOG_DIR"
echo "================================================================="

SSH_OPTS="-i $SSH_KEY -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=5"

echo "[1/7] Fetching Kernel dmesg..."
ssh $SSH_OPTS root@"$DEVICE_IP" "dmesg" > "$TARGET_LOG_DIR/dmesg.log" 2>/dev/null || echo "[!] Failed to fetch dmesg"

echo "[2/7] Fetching Early Boot Hardware Log..."
ssh $SSH_OPTS root@"$DEVICE_IP" "cat /var/log/native_boot.log" > "$TARGET_LOG_DIR/native_boot_init.log" 2>/dev/null || echo "[!] Failed to fetch init log"

echo "[3/7] Fetching System & Kernel Information..."
ssh $SSH_OPTS root@"$DEVICE_IP" "cat /proc/cmdline ; echo '---' ; cat /proc/partitions ; echo '---' ; uname -a" > "$TARGET_LOG_DIR/system_info.log" 2>/dev/null || true

echo "[4/7] Fetching Display & Backlight State..."
ssh $SSH_OPTS root@"$DEVICE_IP" "ls -la /sys/class/backlight/* 2>/dev/null ; cat /sys/class/backlight/*/brightness 2>/dev/null ; ls -la /sys/class/drm/* 2>/dev/null" > "$TARGET_LOG_DIR/display_backlight.log" 2>/dev/null || true

echo "[5/7] Fetching Touchscreen & Input Devices..."
ssh $SSH_OPTS root@"$DEVICE_IP" "cat /proc/bus/input/devices 2>/dev/null" > "$TARGET_LOG_DIR/input_devices.log" 2>/dev/null || true

echo "[6/7] Fetching Thermal & Power Subsystem..."
ssh $SSH_OPTS root@"$DEVICE_IP" "
for t in /sys/class/thermal/thermal_zone*; do
    [ -f \"\$t/temp\" ] && echo \"\$(basename \"\$t\") (\$(cat \"\$t/type\" 2>/dev/null)): \$(cat \"\$t/temp\" 2>/dev/null)\"
done
echo '--- Power Supply ---'
for p in /sys/class/power_supply/*; do
    [ -d \"\$p\" ] && echo \"Device: \$(basename \"\$p\") | Status: \$(cat \"\$p/status\" 2>/dev/null) | Capacity: \$(cat \"\$p/capacity\" 2>/dev/null)%\"
done
" > "$TARGET_LOG_DIR/power_thermal.log" 2>/dev/null || true

echo "[7/7] Fetching Storage & Block Layer Devices..."
ssh $SSH_OPTS root@"$DEVICE_IP" "ls -la /dev/sda* /dev/sdd* /dev/block/ 2>/dev/null" > "$TARGET_LOG_DIR/storage_blocks.log" 2>/dev/null || true

echo "================================================================="
echo "Native boot logs successfully saved to:"
echo "$TARGET_LOG_DIR"
ls -lh "$TARGET_LOG_DIR"
echo "================================================================="
