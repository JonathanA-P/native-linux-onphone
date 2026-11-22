#!/usr/bin/env bash
# scripts/check_kernel_config.sh — Audit mandatory kernel config flags
set -e
CONFIG_FILE="/home/jonathan/Documents/Projects/Apps/a70/native-linux/artifacts/kernel/build.config"

MANDATORY_FLAGS=(
    "CONFIG_ARM64=y"
    "CONFIG_SCSI_UFS_QCOM=y"
    "CONFIG_EXT4_FS=y"
    "CONFIG_DEVTMPFS=y"
    "CONFIG_DEVTMPFS_MOUNT=y"
    "CONFIG_USER_NS=y"
    "CONFIG_PID_NS=y"
    "CONFIG_IPC_NS=y"
    "CONFIG_CGROUP_DEVICE=y"
    "CONFIG_USB_CONFIGFS=y"
    "CONFIG_USB_CONFIGFS_RNDIS=y"
    "CONFIG_TOUCHSCREEN_IST40XX=y"
    "CONFIG_DRM_MSM=y"
)

echo "=== Auditing Kernel Configuration ($CONFIG_FILE) ==="
ALL_PASS=true
for flag in "${MANDATORY_FLAGS[@]}"; do
    if grep -q "^$flag" "$CONFIG_FILE"; then
        echo "[PASS] $flag"
    else
        echo "[FAIL] Missing: $flag"
        ALL_PASS=false
    fi
done

if [ "$ALL_PASS" = true ]; then
    echo "=== All mandatory kernel options verified. ==="
else
    echo "=== Some mandatory kernel options failed! ==="
    exit 1
fi
