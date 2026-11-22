#!/usr/bin/env bash
set -e

WORKSPACE_DIR="/home/jonathan/Documents/Projects/Apps/a70/native-linux"
KERNEL_DIR="$WORKSPACE_DIR/kernel/sm6150-kernel"
ARTIFACTS_DIR="$WORKSPACE_DIR/artifacts/kernel"
LOGS_DIR="$WORKSPACE_DIR/logs"

mkdir -p "$ARTIFACTS_DIR" "$LOGS_DIR"

echo "================================================================="
echo "  SAMSUNG GALAXY A70 (SM6150) — KERNEL BUILD PIPELINE"
echo "  Target: Image.gz-dtb + dtbo.img (Clang 14.0.6 r450784d)"
echo "================================================================="

LOG_FILE="$LOGS_DIR/kernel_build_$(date +%Y%m%d_%H%M%S).log"
echo "Log file: $LOG_FILE"

podman run --rm \
    -v "$KERNEL_DIR:/buildd/sources:Z" \
    -v "$ARTIFACTS_DIR:/buildd/artifacts:Z" \
    -w /buildd/sources \
    quay.io/droidian/build-essential:current-amd64 bash -c "
set -e
echo '=== 1. INSTALLING COMPILER & BUILD DEPENDENCIES ==='
apt-get update -qq
apt-get install --no-install-recommends -y -qq \
    bc kmod cpio bison flex libssl-dev lz4 rsync \
    binutils-aarch64-linux-gnu gcc-4.9-aarch64-linux-android \
    libgcc-4.9-dev-aarch64-linux-android-cross \
    clang-android-14.0-r450784d mkdtboimg > /dev/null

printf '#!/bin/sh\necho false\n' > /usr/local/bin/secgetspf && chmod +x /usr/local/bin/secgetspf

export ARCH=arm64
export SUBARCH=arm64
export PATH=/usr/lib/llvm-android-14.0-r450784d/bin:\$PATH

MAKE_OPTS=\"ARCH=arm64 CC=clang CLANG_TRIPLE=aarch64-linux-gnu- CROSS_COMPILE=aarch64-linux-gnu-\"

echo '=== 2. COMPILER VERIFICATION ==='
clang --version | head -n 1
aarch64-linux-gnu-as --version | head -n 1

echo '=== 3. CONFIGURING KERNEL (a70q_droidian_defconfig) ==='
make \$MAKE_OPTS a70q_droidian_defconfig

echo '=== 4. COMPILING KERNEL (Image.gz-dtb) ==='
make \$MAKE_OPTS -j\$(nproc) Image.gz-dtb

echo '=== 5. BUILDING DTBO OVERLAYS ==='
make \$MAKE_OPTS -j\$(nproc) dtbos || make \$MAKE_OPTS -j\$(nproc) arch/arm64/boot/dts/samsung/

if ls arch/arm64/boot/dts/samsung/*.dtbo 1> /dev/null 2>&1; then
    echo 'Creating dtbo.img with mkdtboimg...'
    mkdtboimg create /buildd/artifacts/dtbo.img arch/arm64/boot/dts/samsung/*.dtbo
fi

echo '=== 6. COPYING ARTIFACTS ==='
cp arch/arm64/boot/Image.gz-dtb /buildd/artifacts/Image.gz-dtb
cp arch/arm64/boot/Image.gz /buildd/artifacts/Image.gz || true
cp .config /buildd/artifacts/build.config

echo '=== 7. VERIFYING ARTIFACTS ==='
ls -lh /buildd/artifacts/
file /buildd/artifacts/Image.gz-dtb || true
" 2>&1 | tee "$LOG_FILE"

echo "Build complete. Artifacts saved in $ARTIFACTS_DIR"
