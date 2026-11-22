#!/usr/bin/env bash
# scripts/connect_debian.sh — Unified Connector for Native Linux / Debian on Samsung Galaxy A70
set -e

SERIAL="RR8M50J37GY"
PORT_SSH=2222
PORT_VNC=5903

# 1. Forward ADB ports
adb -s "$SERIAL" forward tcp:$PORT_SSH tcp:$PORT_SSH >/dev/null 2>&1 || true
adb -s "$SERIAL" forward tcp:$PORT_VNC tcp:$PORT_VNC >/dev/null 2>&1 || true

# 2. Check device state
DEV_STATE=$(adb -s "$SERIAL" get-state 2>/dev/null || echo "offline")

if [ "$DEV_STATE" = "recovery" ]; then
    echo "=== Samsung Galaxy A70 — Native Linux (Bootloader Recovery Mode) ==="
    # Ensure Debian mounts and sshd are alive
    adb -s "$SERIAL" shell "
        mkdir -p /data
        mount -t f2fs /dev/block/bootdevice/by-name/userdata /data 2>/dev/null || mount /dev/sda29 /data 2>/dev/null
        if [ -d /data/debian ]; then
            mountpoint -q /data/debian/proc || mount -t proc proc /data/debian/proc 2>/dev/null
            mountpoint -q /data/debian/sys || mount -t sysfs sys /data/debian/sys 2>/dev/null
            mountpoint -q /data/debian/dev || mount --bind /dev /data/debian/dev 2>/dev/null
            mountpoint -q /data/debian/dev/pts || mount -t devpts devpts /data/debian/dev/pts 2>/dev/null
            chmod 755 /data/debian/run/sshd 2>/dev/null
            chroot /data/debian /usr/bin/env PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin /usr/sbin/sshd -p $PORT_SSH 2>/dev/null || true
        fi
    " >/dev/null 2>&1 || true
elif [ "$DEV_STATE" = "device" ]; then
    echo "=== Samsung Galaxy A70 — Native Linux (Android Runtime Mode) ==="
fi

if [ "$1" == "--adb" ]; then
    exec adb -s "$SERIAL" shell
elif [ "$1" == "--vnc" ]; then
    if command -v vncviewer >/dev/null 2>&1; then
        exec vncviewer 127.0.0.1:$PORT_VNC
    elif command -v remote-viewer >/dev/null 2>&1; then
        exec remote-viewer vnc://127.0.0.1:$PORT_VNC
    else
        echo "Connect VNC viewer to 127.0.0.1:$PORT_VNC"
    fi
elif [ -n "$1" ]; then
    exec ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR -p $PORT_SSH root@127.0.0.1 "$@"
else
    echo "  Connecting to Debian GNU/Linux root shell..."
    exec ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR -p $PORT_SSH root@127.0.0.1
fi
