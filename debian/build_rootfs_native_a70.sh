#!/usr/bin/env bash
set -e

# ==============================================================================
# Genuine Debian 13 (Trixie) ARM64 Native Bootstrap Script for Samsung Galaxy A70
# Runs natively on aarch64 environment with direct package execution.
# ==============================================================================

WORK_DIR="/home/debianuser/build_debian13"
TARGET_DIR="$WORK_DIR/rootfs"
OUTPUT_TAR="/home/debianuser/debian-13-trixie-arm64.tar.xz"

echo "=== 1. PREPARING WORKSPACE ==="
sudo rm -rf "$WORK_DIR"
mkdir -p "$TARGET_DIR"

echo "=== 2. RUNNING MMDEBSTRAP (NATIVE ARM64 DEBIAN 13 TRIXIE) ==="
sudo mmdebstrap \
    --architectures=arm64 \
    --include=systemd,systemd-sysv,udev,kmod,iproute2,net-tools,isc-dhcp-client,openssh-server,sudo,curl,wget,nano,locales,tzdata,ca-certificates,dbus,libinput-bin,polkitd,procps,htop \
    trixie \
    "$TARGET_DIR" \
    http://deb.debian.org/debian

echo "=== 3. CONFIGURING SYSTEM SETTINGS & USERS ==="
# Hostname & hosts
echo "a70-debian" | sudo tee "$TARGET_DIR/etc/hostname" > /dev/null
sudo tee "$TARGET_DIR/etc/hosts" > /dev/null << 'EOF'
127.0.0.1   localhost
127.0.1.1   a70-debian
::1         localhost ip6-localhost ip6-loopback
EOF

# fstab
sudo tee "$TARGET_DIR/etc/fstab" > /dev/null << 'EOF'
LABEL=DEBIAN_ROOT   /           ext4    errors=remount-ro,noatime,barrier=1   0 1
tmpfs               /tmp        tmpfs   defaults,nodev,nosuid                 0 0
EOF

# Users & Passwords
sudo chroot "$TARGET_DIR" /bin/bash -c "
echo root:root | chpasswd
useradd -m -s /bin/bash -G sudo,video,audio,input,render debian 2>/dev/null || true
echo debian:debian | chpasswd
systemctl enable ssh || true
"

# Sudoers
sudo mkdir -p "$TARGET_DIR/etc/sudoers.d"
echo 'debian ALL=(ALL) NOPASSWD:ALL' | sudo tee "$TARGET_DIR/etc/sudoers.d/010_debian-nopasswd" > /dev/null
sudo chmod 0440 "$TARGET_DIR/etc/sudoers.d/010_debian-nopasswd"

# Network usb0 static IP configuration for RNDIS gadget console
sudo mkdir -p "$TARGET_DIR/etc/network/interfaces.d"
sudo tee "$TARGET_DIR/etc/network/interfaces.d/usb0" > /dev/null << 'EOF'
auto usb0
iface usb0 inet static
    address 192.168.42.2
    netmask 255.255.255.0
    gateway 192.168.42.1
EOF

# SSH Config
sudo mkdir -p "$TARGET_DIR/etc/ssh/sshd_config.d"
sudo tee "$TARGET_DIR/etc/ssh/sshd_config.d/01-debian-dualboot.conf" > /dev/null << 'EOF'
PasswordAuthentication yes
PermitRootLogin yes
EOF

# Locale
echo "en_US.UTF-8 UTF-8" | sudo tee "$TARGET_DIR/etc/locale.gen" > /dev/null
sudo chroot "$TARGET_DIR" locale-gen || true

# Cleanup apt cache to minimize image size
sudo chroot "$TARGET_DIR" apt-get clean || true
sudo rm -rf "$TARGET_DIR/var/lib/apt/lists/*"

echo "=== 4. PACKAGING TARBALL ==="
sudo rm -f "$OUTPUT_TAR"
sudo tar -C "$TARGET_DIR" -cJf "$OUTPUT_TAR" .
sudo chown debianuser:debianuser "$OUTPUT_TAR"

echo "=== ROOTFS BUILD SUCCESSFUL ==="
ls -lh "$OUTPUT_TAR"
