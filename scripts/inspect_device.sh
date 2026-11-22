#!/usr/bin/env bash
# scripts/inspect_device.sh — Deep inspection of connected Samsung Galaxy A70
set -e

echo "=== Samsung Galaxy A70 Device Inspection ==="
adb devices -l
echo ""
echo "=== Android System Properties ==="
adb shell "getprop ro.product.model ; getprop ro.product.device ; getprop ro.boot.bootloader ; getprop ro.build.version.release ; getprop ro.crypto.state ; getprop ro.crypto.type"
echo ""
echo "=== Storage & Mounts ==="
adb shell "su -c 'df -h /data ; df -T /data ; mount | grep -E \"userdata|metadata|system|vendor\"'"
echo ""
echo "=== Display & Backlight Nodes ==="
adb shell "su -c 'cat /sys/class/backlight/backlight/max_brightness 2>/dev/null ; readlink -f /sys/class/backlight/backlight/device 2>/dev/null'"
echo ""
echo "=== Touchscreen Input Nodes ==="
adb shell "su -c 'cat /proc/bus/input/devices | grep -A 8 sec_touchscreen'"
