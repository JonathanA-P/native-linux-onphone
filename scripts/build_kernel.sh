#!/usr/bin/env bash
# scripts/build_kernel.sh — Reproducible container kernel compilation
set -e

WORKSPACE_DIR="/home/jonathan/Documents/Projects/Apps/a70/native-linux"
ARTIFACTS_DIR="$WORKSPACE_DIR/artifacts/kernel"
KERNEL_DIR="$WORKSPACE_DIR/kernel/sm6150-kernel"

mkdir -p "$ARTIFACTS_DIR"

echo "=== Building Samsung SM6150 Kernel (Image.gz-dtb & dtbo.img) ==="
podman run --rm --privileged \
    -v "$KERNEL_DIR:/buildd/kernel:Z" \
    -v "$ARTIFACTS_DIR:/buildd/artifacts:Z" \
    -w /buildd/kernel \
    quay.io/droidian/build-essential:current-amd64 bash -c '
set -e
export ARCH=arm64
export SUBARCH=arm64
export CC=clang
export CLANG_TRIPLE=aarch64-linux-gnu-
export CROSS_COMPILE=aarch64-linux-gnu-

make a70q_droidian_defconfig
make -j$(nproc) Image.gz-dtb
mkdtboimg create /buildd/artifacts/dtbo.img arch/arm64/boot/dts/samsung/*.dtbo
cp arch/arm64/boot/Image.gz-dtb /buildd/artifacts/Image.gz-dtb
cp arch/arm64/boot/Image.gz /buildd/artifacts/Image.gz
cp .config /buildd/artifacts/build.config
ls -lh /buildd/artifacts/
'
