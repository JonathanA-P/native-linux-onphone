#!/usr/bin/env bash
# scripts/connect_diagnostic_device.sh — Connect Fedora host to A70 diagnostic USB RNDIS console
set -e

HOST_IP="192.168.42.1/24"
DEVICE_IP="192.168.42.2"
SSH_KEY="$HOME/.ssh/id_ed25519"

echo "================================================================="
echo "  SAMSUNG GALAXY A70 — DIAGNOSTIC NETWORK CONSOLE CONNECTOR"
echo "  Target Device IP: $DEVICE_IP"
echo "  Host Assigned IP: $HOST_IP"
echo "================================================================="

# 1. Identify USB network interface
echo "[1/4] Scanning for USB RNDIS network interfaces..."
IFACE=""
# Look for usb0 or enx* interfaces matching Qualcomm RNDIS
for dev in $(ip -o link show | awk -F': ' '{print $2}'); do
    if [[ "$dev" =~ ^(usb|enx|enp[0-9]+s[0-9]+u) ]]; then
        IFACE="$dev"
        break
    fi
done

if [ -z "$IFACE" ]; then
    echo "[!] No USB network interface automatically detected."
    echo "    Available interfaces:"
    ip -br link
    read -rp "Enter USB network interface manually (e.g. usb0): " IFACE
fi

if [ -z "$IFACE" ]; then
    echo "ERROR: No interface selected. Exiting."
    exit 1
fi

echo "Selected interface: $IFACE"

# 2. Configure host IP address
echo "[2/4] Configuring host IP $HOST_IP on $IFACE..."
if ! ip addr show dev "$IFACE" 2>/dev/null | grep -q "192.168.42.1"; then
    nmcli dev modify "$IFACE" ipv4.addresses 192.168.42.1/24 ipv4.method manual 2>/dev/null || \
    sudo -n ip addr add "$HOST_IP" dev "$IFACE" 2>/dev/null || \
    sudo ip addr add "$HOST_IP" dev "$IFACE" 2>/dev/null || true
fi
sudo -n ip link set dev "$IFACE" up 2>/dev/null || ip link set dev "$IFACE" up 2>/dev/null || true

# 3. Test connectivity (Ping)
echo "[3/4] Testing link to phone ($DEVICE_IP)..."
if ping -c 3 -W 2 "$DEVICE_IP" > /dev/null 2>&1; then
    echo ">>> LINK ACTIVE: Phone is responding on $DEVICE_IP! <<<"
else
    echo "[WARN] Phone ($DEVICE_IP) did not answer ping yet."
    echo "       Device may still be booting or USB gadget negotiating."
fi

# 4. Attempt SSH connection
echo "[4/4] Attempting authenticated SSH connection to phone console..."
if [ -f "$SSH_KEY" ]; then
    ssh -i "$SSH_KEY" -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=5 root@"$DEVICE_IP"
else
    ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=5 root@"$DEVICE_IP"
fi
