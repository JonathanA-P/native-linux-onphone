#!/usr/bin/env bash
# scripts/restore_stock_recovery.sh — Restore verified stock recovery via Heimdall
set -e

BACKUP="/home/jonathan/Backups/A70-Linux-Migration/partitions/stock_recovery.img"
EXPECTED_SHA="049fb22fec89a456a4a8c540b0195508e617b51ae0fdbc3f752ee2fc10d7fd15"
HEIMDALL="$HOME/.local/bin/heimdall"

echo "================================================================="
echo "  RESTORE ORIGINAL STOCK RECOVERY — SAMSUNG GALAXY A70"
echo "  Target: RECOVERY Partition"
echo "  Backup: $BACKUP"
echo "================================================================="

if [ ! -f "$BACKUP" ]; then
    echo "ERROR: Stock recovery backup not found at $BACKUP"
    exit 1
fi

ACTUAL_SHA=$(sha256sum "$BACKUP" | awk '{print $1}')
if [ "$ACTUAL_SHA" != "$EXPECTED_SHA" ]; then
    echo "ERROR: Backup checksum mismatch! Aborting."
    exit 1
fi
echo "Backup SHA256 verified: $ACTUAL_SHA"

echo "Waiting for device in Samsung Download Mode..."
while ! $HEIMDALL detect >/dev/null 2>&1; do
    sleep 1
done
echo "Device detected in Download Mode!"

echo "Flashing verified stock recovery..."
$HEIMDALL flash --RECOVERY "$BACKUP"

echo ">>> SUCCESS: Stock recovery restored! <<<"
