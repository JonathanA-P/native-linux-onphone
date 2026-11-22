#!/usr/bin/env bash
set -e

# ==============================================================================
# Genuine Debian 13 (Trixie) ARM64 Rootfs Builder for Samsung Galaxy A70
# Builds an independently reproducible, genuine Debian 13 userspace.
# ==============================================================================

WORKSPACE_DIR="/home/jonathan/Documents/Projects/Apps/a70/native-linux"
ARTIFACTS_DIR="$WORKSPACE_DIR/artifacts/rootfs"
BUILD_DIR="$WORKSPACE_DIR/debian"
ROOTFS_IMG="$ARTIFACTS_DIR/debian-rootfs.img"
ROOTFS_TAR="$ARTIFACTS_DIR/debian-13-trixie-arm64.tar.xz"

mkdir -p "$ARTIFACTS_DIR"

echo "================================================================="
echo "  BUILDING GENUINE DEBIAN 13 (TRIXIE) ARM64 ROOTFS"
echo "  Target: $ROOTFS_TAR & $ROOTFS_IMG"
echo "================================================================="

podman run --rm --privileged \
    -v "$ARTIFACTS_DIR:/output:Z" \
    -v "$BUILD_DIR:/scripts:Z" \
    -w /tmp \
    docker.io/library/debian:bookworm bash /scripts/container_build.sh

echo "Artifacts generated successfully in $ARTIFACTS_DIR"
