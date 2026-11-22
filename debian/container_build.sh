#!/usr/bin/env bash
set -e

export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq mmdebstrap qemu-user-static binfmt-support xz-utils e2fsprogs util-linux > /dev/null

TARGET_DIR=/tmp/rootfs
mkdir -p "$TARGET_DIR"

echo "=== 1. RUNNING MMDEBSTRAP FOR DEBIAN 13 TRIXIE (ARM64) ==="
mmdebstrap \
    --architectures=arm64 \
    --include=systemd,systemd-sysv,udev,kmod,iproute2,net-tools,isc-dhcp-client,openssh-server,sudo,curl,wget,nano,locales,tzdata,ca-certificates,dbus,libinput-bin,polkitd,procps,htop \
    trixie \
    "$TARGET_DIR" \
    http://deb.debian.org/debian

echo "=== 2. CONFIGURING USERS, HOSTNAME & SYSTEM SETTINGS ==="
# Ensure qemu is available in chroot
cp /usr/bin/qemu-aarch64-static "$TARGET_DIR/usr/bin/" 2>/dev/null || true

# Set Hostname
echo "a70-debian" > "$TARGET_DIR/etc/hostname"

# Set Hosts
cat << 'EOF' > "$TARGET_DIR/etc/hosts"
127.0.0.1   localhost
127.0.1.1   a70-debian
::1         localhost ip6-localhost ip6-loopback
EOF

# Set fstab
cat << 'EOF' > "$TARGET_DIR/etc/fstab"
# /etc/fstab: static file system information.
LABEL=DEBIAN_ROOT   /           ext4    errors=remount-ro,noatime,barrier=1   0 1
tmpfs               /tmp        tmpfs   defaults,nodev,nosuid                 0 0
EOF

# Set root password and create user debian
chroot "$TARGET_DIR" /bin/bash -c "
echo root:root | chpasswd
useradd -m -s /bin/bash -G sudo,video,audio,input,render debian 2>/dev/null || true
echo debian:debian | chpasswd
systemctl enable ssh || true
"

echo 'debian ALL=(ALL) NOPASSWD:ALL' > "$TARGET_DIR/etc/sudoers.d/010_debian-nopasswd"
chmod 0440 "$TARGET_DIR/etc/sudoers.d/010_debian-nopasswd"

# Configure networking: USB Gadget interface usb0 with static IP
mkdir -p "$TARGET_DIR/etc/network/interfaces.d/"
cat << 'EOF' > "$TARGET_DIR/etc/network/interfaces.d/usb0"
auto usb0
iface usb0 inet static
    address 192.168.42.2
    netmask 255.255.255.0
    gateway 192.168.42.1
EOF

# Configure SSH daemon to permit password login
sed -i 's/#PasswordAuthentication yes/PasswordAuthentication yes/' "$TARGET_DIR/etc/ssh/sshd_config" || true

# Generate default locale
echo "en_US.UTF-8 UTF-8" > "$TARGET_DIR/etc/locale.gen"
chroot "$TARGET_DIR" locale-gen || true

# Remove qemu binary before packaging
rm -f "$TARGET_DIR/usr/bin/qemu-aarch64-static"

echo "=== 3. PACKAGING ROOTFS TARBALL ==="
tar -C "$TARGET_DIR" -cJf /output/debian-13-trixie-arm64.tar.xz .

echo "=== 4. GENERATING RAW EXT4 SPARSE ROOTFS IMAGE ==="
dd if=/dev/zero of=/output/debian-rootfs.img bs=1M count=0 seek=4096
mkfs.ext4 -F -L DEBIAN_ROOT /output/debian-rootfs.img

MOUNT_DIR=/tmp/mnt_rootfs
mkdir -p "$MOUNT_DIR"
mount -o loop /output/debian-rootfs.img "$MOUNT_DIR"
cp -a "$TARGET_DIR"/. "$MOUNT_DIR"/
umount "$MOUNT_DIR"

echo "=== ROOTFS BUILD COMPLETE ==="
ls -lh /output/
