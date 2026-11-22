#!/usr/bin/env bash
set -e

# ==============================================================================
# Samsung Galaxy A70 (SM6150) — Secure Initramfs Build Pipeline
# Features: AArch64 Busybox + Dropbear SSH (Public-Key Only) + RNDIS + WLED Backlight
# Zero Unauthenticated Telnet / Zero Passwordless Root
# ==============================================================================

WORKSPACE_DIR="/home/jonathan/Documents/Projects/Apps/a70/native-linux"
INITRAMFS_DIR="$WORKSPACE_DIR/initramfs"
ARTIFACTS_DIR="$WORKSPACE_DIR/artifacts/boot"
OUTPUT_CPIO="$ARTIFACTS_DIR/initramfs.cpio.gz"
HOST_PUBKEY=$(cat ~/.ssh/id_*.pub 2>/dev/null | head -n 1)

mkdir -p "$ARTIFACTS_DIR"

echo "================================================================="
echo "  SAMSUNG GALAXY A70 (SM6150) — SECURE INITRAMFS BUILD PIPELINE"
echo "  Target: $OUTPUT_CPIO"
echo "  Authorized Pubkey: $HOST_PUBKEY"
echo "================================================================="

podman run --rm --privileged \
    -v "$INITRAMFS_DIR:/buildd/initramfs:Z" \
    -v "$ARTIFACTS_DIR:/buildd/artifacts:Z" \
    -e "HOST_PUBKEY=$HOST_PUBKEY" \
    -w /tmp \
    docker.io/library/debian:bookworm bash -c '
set -e
dpkg --add-architecture arm64
apt-get update -qq
apt-get install -y -qq busybox-static dropbear-bin cpio gzip util-linux file > /dev/null

BUILD_ROOT=/tmp/initramfs_root
mkdir -p "$BUILD_ROOT"/{bin,sbin,dev,proc,sys,sysroot,mnt_data,sys/kernel/config,etc/dropbear,root/.ssh,var/run}

# 1. Download and unpack ARM64 static busybox and dropbear
mkdir -p /tmp/arm64_deb
cd /tmp/arm64_deb
apt-get download -qq busybox-static:arm64 dropbear-bin:arm64 libcrypt1:arm64 libc6:arm64 zlib1g:arm64 libtomcrypt1:arm64 libtommath1:arm64
for f in *.deb; do
    dpkg-deb -x "$f" /tmp/arm64_pkg
done

# Copy busybox
cp /tmp/arm64_pkg/bin/busybox "$BUILD_ROOT/bin/busybox"
chmod 755 "$BUILD_ROOT/bin/busybox"

# Symlinks for busybox
for app in $(busybox --list); do
    if [ "$app" != "busybox" ]; then
        ln -sf busybox "$BUILD_ROOT/bin/$app"
    fi
done

# Copy dropbear binaries & required shared libraries
cp /tmp/arm64_pkg/usr/sbin/dropbear "$BUILD_ROOT/sbin/dropbear" 2>/dev/null || cp /tmp/arm64_pkg/usr/bin/dropbear "$BUILD_ROOT/sbin/dropbear"
cp /tmp/arm64_pkg/usr/bin/dropbearkey "$BUILD_ROOT/bin/dropbearkey"
chmod 755 "$BUILD_ROOT/sbin/dropbear" "$BUILD_ROOT/bin/dropbearkey"

# Copy ARM64 dynamic linker and libraries for dropbear
mkdir -p "$BUILD_ROOT"/lib "$BUILD_ROOT"/usr/lib "$BUILD_ROOT"/lib64
cp -a /tmp/arm64_pkg/lib/* "$BUILD_ROOT/lib/" 2>/dev/null || true
cp -a /tmp/arm64_pkg/usr/lib/* "$BUILD_ROOT/usr/lib/" 2>/dev/null || true

# Generate Dropbear host key inside initramfs using qemu or host dropbearkey
dropbearkey -t ed25519 -f "$BUILD_ROOT/etc/dropbear/dropbear_ed25519_host_key"

# Install user authorized public key
if [ -n "$HOST_PUBKEY" ]; then
    echo "$HOST_PUBKEY" > "$BUILD_ROOT/root/.ssh/authorized_keys"
    chmod 700 "$BUILD_ROOT/root/.ssh"
    chmod 600 "$BUILD_ROOT/root/.ssh/authorized_keys"
    echo "Installed host authorized public key for root login."
fi

# Create /etc/passwd and /etc/group for dropbear root user
cat << EOF > "$BUILD_ROOT/etc/passwd"
root:x:0:0:root:/root:/bin/sh
EOF
cat << EOF > "$BUILD_ROOT/etc/group"
root:x:0:
EOF

# Copy custom init
cp /buildd/initramfs/init "$BUILD_ROOT/init"
chmod 755 "$BUILD_ROOT/init"

# Package CPIO archive
cd "$BUILD_ROOT"
find . | cpio -o -H newc | gzip -9 > /buildd/artifacts/initramfs.cpio.gz

echo "=== Initramfs verification ==="
file "$BUILD_ROOT/bin/busybox"
ls -lh /buildd/artifacts/initramfs.cpio.gz
'

echo "Initramfs generated successfully: $OUTPUT_CPIO"
