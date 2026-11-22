#!/usr/bin/env bash
set -e

# ==============================================================================
# Generate Raw Sparse EXT4 Image from Genuine Debian 13 (Trixie) Tarball
# Uses native mke2fs directly on Fedora host without requiring root/loop devices.
# ==============================================================================

WORKSPACE_DIR="/home/jonathan/Documents/Projects/Apps/a70/native-linux"
ARTIFACTS_DIR="$WORKSPACE_DIR/artifacts/rootfs"
ROOTFS_TAR="$ARTIFACTS_DIR/debian-13-trixie-arm64.tar.xz"
ROOTFS_IMG="$ARTIFACTS_DIR/debian-rootfs.img"
TMP_DIR="/tmp/debian13_trixie_unpack_$$"

if [ ! -f "$ROOTFS_TAR" ]; then
    echo "Error: $ROOTFS_TAR not found!"
    exit 1
fi

echo "=== 1. EXTRACTING ROOTFS TARBALL TO TEMPORARY DIRECTORY ==="
mkdir -p "$TMP_DIR"
tar -xf "$ROOTFS_TAR" -C "$TMP_DIR"

echo "=== 2. CREATING 4 GIB EXT4 SPARSE IMAGE VIA MKE2FS ==="
rm -f "$ROOTFS_IMG"
mke2fs -t ext4 -F -L DEBIAN_ROOT -d "$TMP_DIR" -b 4096 "$ROOTFS_IMG" 4G

echo "=== 3. CLEANING UP TEMPORARY DIRECTORY ==="
rm -rf "$TMP_DIR"

echo "=== 4. VERIFYING GENERATED ROOTFS IMAGE ==="
ls -lh "$ROOTFS_IMG"
file "$ROOTFS_IMG"
sha256sum "$ROOTFS_IMG" > "$ROOTFS_IMG.sha256"
cat "$ROOTFS_IMG.sha256"
