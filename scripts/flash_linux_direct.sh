#!/bin/bash
# Samsung Galaxy A70 (a70q / SM-A705F) - Direct Linux Boot Heimdall Flasher
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

BOOT_IMG="${1:-$BASE_DIR/artifacts/boot/a70-linux-boot-direct.img}"
DTBO_IMG="${2:-$BASE_DIR/artifacts/boot/dtbo_lineage.img}"
REC_IMG="${3:-$BASE_DIR/artifacts/boot/a70-linux-recovery-lineage.img}"
VBMETA_IMG="${4:-$BASE_DIR/artifacts/boot/vbmeta.img}"

echo "=========================================================="
echo " Samsung Galaxy A70 (a70q) - Direct Linux Flashing Suite"
echo "=========================================================="
echo "Boot Image:     $BOOT_IMG"
echo "DTBO Image:     $DTBO_IMG"
echo "Recovery Image: $REC_IMG"
echo "VBMeta Image:   $VBMETA_IMG"
echo ""
echo "[*] Waiting for Samsung Galaxy A70 in Download Mode..."
echo "    (Hold Volume Down + Volume Up, plug in USB cable to PC, then press Volume Up to Continue)"

while true; do
    if heimdall detect 2>&1 | grep -q "Device detected"; then
        echo "[+] Device detected in Download Mode!"
        break
    fi
    sleep 0.5
done

echo "[*] Flashing direct Linux boot suite via Heimdall..."
heimdall flash \
    --BOOT "$BOOT_IMG" \
    --DTBO "$DTBO_IMG" \
    --RECOVERY "$REC_IMG" \
    --VBMETA "$VBMETA_IMG"

echo "[✓] Flash complete! Phone is rebooting directly into Native Linux."
