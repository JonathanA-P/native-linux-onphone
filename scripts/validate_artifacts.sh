#!/usr/bin/env bash
# scripts/validate_artifacts.sh — Inspect all generated native boot artifacts
set -e
ARTIFACTS_DIR="/home/jonathan/Documents/Projects/Apps/a70/native-linux/artifacts"

echo "=== Verifying Artifact File Sizes & Types ==="
ls -lh "$ARTIFACTS_DIR"/kernel/ "$ARTIFACTS_DIR"/boot/ "$ARTIFACTS_DIR"/rootfs/
echo ""
file "$ARTIFACTS_DIR"/kernel/Image.gz-dtb
file "$ARTIFACTS_DIR"/kernel/dtbo.img
file "$ARTIFACTS_DIR"/boot/initramfs.cpio.gz
file "$ARTIFACTS_DIR"/boot/debian-boot.img
file "$ARTIFACTS_DIR"/rootfs/debian-13-trixie-arm64.tar.xz
file "$ARTIFACTS_DIR"/rootfs/debian-rootfs.img
