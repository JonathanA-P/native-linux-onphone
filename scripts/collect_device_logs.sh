#!/usr/bin/env bash
# scripts/collect_device_logs.sh — Gather device logs from connected A70
set -e
LOGS_DIR="/home/jonathan/Documents/Projects/Apps/a70/native-linux/logs"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

mkdir -p "$LOGS_DIR"

echo "=== Collecting Kernel dmesg ==="
adb shell "su -c 'dmesg'" > "$LOGS_DIR/dmesg_$TIMESTAMP.log" 2>/dev/null || true

echo "=== Collecting Recovery logs ==="
adb shell "su -c 'cat /cache/recovery/last_log'" > "$LOGS_DIR/recovery_last_log_$TIMESTAMP.log" 2>/dev/null || true

echo "=== Collecting Console pstore ==="
adb shell "su -c 'cat /sys/fs/pstore/console-ramoops-0 2>/dev/null || cat /sys/fs/pstore/dmesg-ramoops-0 2>/dev/null'" > "$LOGS_DIR/pstore_$TIMESTAMP.log" 2>/dev/null || true

echo "Logs collected under $LOGS_DIR/ with timestamp $TIMESTAMP"
ls -lh "$LOGS_DIR"/*"$TIMESTAMP"*
