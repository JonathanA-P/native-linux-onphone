#!/usr/bin/env bash
# build_bootimg.sh — Package Samsung Galaxy A70 (a70q / SM6150) Native Linux Boot Image
# Fixes:
# 1. Calculates exact file offset for --recovery_dtbo required by Samsung S-Boot (magic 0x1eabb7d7)
# 2. Removes earlycon=msm_geni_serial,0x880000 (which hard-faulted due to unclocked UART)
# 3. Uses console=null and nokaslr matching stock crDroid kernel cmdline
# 4. Sets header v1 with board SRPRL06C005
set -e

WORKSPACE_DIR="/home/jonathan/Documents/Projects/Apps/a70/native-linux"
KERNEL_IMG="/tmp/stock_recovery_unpacked/kernel"
[ -f "$KERNEL_IMG" ] || KERNEL_IMG="$WORKSPACE_DIR/artifacts/kernel/Image.gz-dtb"
DTBO_IMG="$WORKSPACE_DIR/device/recovery_dtbo"
RAMDISK_IMG="$WORKSPACE_DIR/artifacts/boot/initramfs.cpio.gz"
OUTPUT_BOOT_IMG="$WORKSPACE_DIR/artifacts/boot/a70-native-linux-patched.img"
MKBOOTIMG="$WORKSPACE_DIR/kernel/sm6150-kernel/tools/mkbootimg"

if [ ! -f "$KERNEL_IMG" ]; then
    echo "ERROR: Kernel image not found at $KERNEL_IMG"
    exit 1
fi

if [ ! -f "$DTBO_IMG" ]; then
    echo "ERROR: DTBO image not found at $DTBO_IMG"
    exit 1
fi

if [ ! -f "$RAMDISK_IMG" ]; then
    echo "ERROR: Initramfs not found at $RAMDISK_IMG"
    exit 1
fi

PAGE_SIZE=4096
k_size=$(stat -c%s "$KERNEL_IMG")
r_size=$(stat -c%s "$RAMDISK_IMG")

align() {
    local val=$1
    echo $(( (val + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1) ))
}

k_aligned=$(align "$k_size")
r_aligned=$(align "$r_size")
DTBO_OFFSET=$(( PAGE_SIZE + k_aligned + r_aligned ))
DTBO_OFFSET_HEX=$(printf "0x%x" "$DTBO_OFFSET")

CMDLINE="console=null androidboot.hardware=qcom androidboot.memcg=1 lpm_levels.sleep_disabled=1 video=vfb:640x400,bpp=32,memsize=3072000 msm_rtb.filter=0x237 service_locator.enable=1 swiotlb=1 androidboot.usbcontroller=a600000.dwc3 firmware_class.path=/vendor/firmware_mnt/image nokaslr printk.devkmsg=on loop.max_part=7 androidboot.avb_version=1.0 androidboot.vbmeta.avb_version=1.0 androidboot.boot_devices=soc/1d84000.ufshc"

echo "================================================================="
echo "  PACKAGING FULLY PATCHED SAMSUNG A70 RECOVERY BOOT IMAGE"
echo "  Kernel:         $KERNEL_IMG ($k_size bytes)"
echo "  Ramdisk:        $RAMDISK_IMG ($r_size bytes)"
echo "  Recovery DTBO:  $DTBO_IMG (offset: $DTBO_OFFSET_HEX)"
echo "  Output:         $OUTPUT_BOOT_IMG"
echo "================================================================="

python3 "$MKBOOTIMG" \
    --kernel "$KERNEL_IMG" \
    --ramdisk "$RAMDISK_IMG" \
    --recovery_dtbo "$DTBO_IMG" \
    --recovery_dtbo_offset "$DTBO_OFFSET_HEX" \
    --cmdline "$CMDLINE" \
    --base 0x00000000 \
    --pagesize "$PAGE_SIZE" \
    --kernel_offset 0x00008000 \
    --ramdisk_offset 0x02000000 \
    --tags_offset 0x01e00000 \
    --os_version 13.0.0 \
    --os_patch_level 2026-11-01 \
    --header_version 1 \
    --board SRPRL06C005 \
    -o "$OUTPUT_BOOT_IMG"

echo "Patched boot image successfully created:"
ls -lh "$OUTPUT_BOOT_IMG"
sha256sum "$OUTPUT_BOOT_IMG"
