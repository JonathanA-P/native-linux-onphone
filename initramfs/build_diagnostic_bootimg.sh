#!/usr/bin/env bash
set -e

WORKSPACE_DIR="/home/jonathan/Documents/Projects/Apps/a70/native-linux"
KERNEL_IMG="/tmp/stock_recovery_unpack/kernel.gz"
DTBO_IMG="/tmp/stock_recovery_dtbo.img"
RAMDISK_IMG="$WORKSPACE_DIR/artifacts/boot/initramfs-diagnostic.cpio.gz"
OUTPUT_BOOT_IMG="$WORKSPACE_DIR/artifacts/boot/a70-diagnostic-boot.img"
MKBOOTIMG="$WORKSPACE_DIR/kernel/sm6150-kernel/tools/mkbootimg"

CMDLINE="console=null androidboot.hardware=qcom androidboot.memcg=1 lpm_levels.sleep_disabled=1 video=vfb:640x400,bpp=32,memsize=3072000 msm_rtb.filter=0x237 service_locator.enable=1 swiotlb=1 androidboot.usbcontroller=a600000.dwc3 firmware_class.path=/vendor/firmware_mnt/image nokaslr printk.devkmsg=on loop.max_part=7 androidboot.boot_devices=soc/1d84000.ufshc enforcing=0 androidboot.selinux=permissive init=/init"

python3 "$MKBOOTIMG" \
    --kernel "$KERNEL_IMG" \
    --ramdisk "$RAMDISK_IMG" \
    --recovery_dtbo "$DTBO_IMG" \
    --recovery_dtbo_offset 0x01dd7000 \
    --cmdline "$CMDLINE" \
    --base 0x00000000 \
    --pagesize 4096 \
    --kernel_offset 0x00008000 \
    --ramdisk_offset 0x02000000 \
    --tags_offset 0x01e00000 \
    --os_version 15.0.0 \
    --os_patch_level 2025-10-01 \
    --header_version 1 \
    --board SRPRL06C005 \
    -o "$OUTPUT_BOOT_IMG"

echo "Patched diagnostic boot image ready:"
ls -lh "$OUTPUT_BOOT_IMG"
