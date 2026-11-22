#!/usr/bin/env bash
# scripts/build_rootfs.sh — Bootstrap genuine Debian 13 Trixie arm64 rootfs
set -e
WORKSPACE_DIR="/home/jonathan/Documents/Projects/Apps/a70/native-linux"
ARTIFACTS_DIR="$WORKSPACE_DIR/artifacts/rootfs"
mkdir -p "$ARTIFACTS_DIR"

echo "=== Generating Raw EXT4 Sparse Rootfs Image ==="
$WORKSPACE_DIR/debian/package_ext4_image.sh
