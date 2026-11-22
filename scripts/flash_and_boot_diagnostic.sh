#!/usr/bin/env bash
# scripts/flash_and_boot_diagnostic.sh — Automated flash, reboot, and native Linux console connector
set -e

BOOT_IMG="/home/jonathan/Documents/Projects/Apps/a70/native-linux/artifacts/boot/a70-diagnostic-boot.img"
HEIMDALL="$HOME/.local/bin/heimdall"
LOG_DIR="/home/jonathan/Documents/Projects/Apps/a70/native-linux/logs/native-boot"
HOST_IP="192.168.42.1/24"
DEVICE_IP="192.168.42.2"
SSH_KEY="$HOME/.ssh/id_ed25519"

mkdir -p "$LOG_DIR"

echo "================================================================="
echo "  SAMSUNG GALAXY A70 — FULL NATIVE BOOT DEPLOYMENT & LOGGING"
echo "  Boot Image: $BOOT_IMG"
echo "================================================================="

# Step 1: Wait for device in Samsung Download Mode
echo "[1/5] Waiting for device in Samsung Download Mode..."
while ! $HEIMDALL detect >/dev/null 2>&1; do
    sleep 1
done
echo ">>> Device detected in Download Mode! <<<"

# Step 2: Flash patched diagnostic boot image to RECOVERY partition
echo "[2/5] Flashing patched diagnostic boot image to RECOVERY..."
$HEIMDALL flash --RECOVERY "$BOOT_IMG"
echo ">>> Flash successfully completed! Phone is rebooting into native Linux... <<<"

# Step 3: Monitor for USB Gadget network interface on Fedora host
echo "[3/5] Waiting for phone USB RNDIS network interface..."
IFACE=""
for i in $(seq 1 45); do
    for dev in $(ip -o link show 2>/dev/null | awk -F': ' '{print $2}'); do
        if [[ "$dev" =~ ^(usb|enx|enp[0-9]+s[0-9]+u) ]]; then
            IFACE="$dev"
            break 2
        fi
    done
    sleep 1
done

if [ -z "$IFACE" ]; then
    echo "[!] USB network interface not automatically found within 45s. Listing interfaces:"
    ip -br link
    echo "Check if phone vibrated or screen turned on."
else
    echo ">>> USB Network interface detected: $IFACE <<<"
    
    # Step 4: Configure host IP
    echo "[4/5] Configuring host IP $HOST_IP on $IFACE..."
    sudo -n ip addr add "$HOST_IP" dev "$IFACE" 2>/dev/null || ip addr add "$HOST_IP" dev "$IFACE" 2>/dev/null || true
    sudo -n ip link set dev "$IFACE" up 2>/dev/null || ip link set dev "$IFACE" up 2>/dev/null || true
    
    # Step 5: Wait for ping & collect native boot logs
    echo "[5/5] Checking phone link at $DEVICE_IP..."
    for i in $(seq 1 30); do
        if ping -c 1 -W 1 "$DEVICE_IP" >/dev/null 2>&1; then
            echo ">>> PHONE RESPONDING ON $DEVICE_IP! <<<"
            break
        fi
        sleep 1
    done
    
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)
    LOG_FILE="$LOG_DIR/native_boot_${TIMESTAMP}.log"
    echo "Collecting native diagnostic logs to $LOG_FILE..."
    
    # Try SSH first
    if [ -f "$SSH_KEY" ]; then
        ssh -i "$SSH_KEY" -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=5 root@"$DEVICE_IP" \
            "cat /var/log/native_boot.log; echo '=== DMESG ==='; cat /var/log/dmesg.log" > "$LOG_FILE" 2>/dev/null || true
    fi
    
    # Fallback to telnet if log file is empty
    if [ ! -s "$LOG_FILE" ]; then
        (echo "cat /var/log/native_boot.log; cat /var/log/dmesg.log; exit"; sleep 2) | telnet "$DEVICE_IP" 23 > "$LOG_FILE" 2>/dev/null || true
    fi
    
    if [ -s "$LOG_FILE" ]; then
        echo ">>> SUCCESS: Native Linux logs captured! <<<"
        head -n 25 "$LOG_FILE"
    fi
fi
