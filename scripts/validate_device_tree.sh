#!/usr/bin/env bash
# scripts/validate_device_tree.sh — Inspect DTB and DTBO overlays
set -e
ARTIFACTS_DIR="/home/jonathan/Documents/Projects/Apps/a70/native-linux/artifacts"

echo "=== 1. Appended DTB Inspection ==="
tail -c 400000 "$ARTIFACTS_DIR/kernel/Image.gz-dtb" | strings | grep -E "qcom,sm6150|qcom,board-id|model" | head -n 10
echo ""
echo "=== 2. DTBO Image Table Inspection ==="
python3 -c "
import struct
with open('$ARTIFACTS_DIR/kernel/dtbo.img', 'rb') as f:
    magic = f.read(4)
    total_size, header_size, dt_entry_size, dt_entry_count, dtbo_offset, page_size, version = struct.unpack('>IIIIIII', f.read(28))
    print(f'Magic: {magic}, Entries: {dt_entry_count}, Total Size: {total_size} bytes')
"
