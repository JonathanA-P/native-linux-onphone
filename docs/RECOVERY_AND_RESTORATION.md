# Samsung Galaxy A70 — Recovery, Restoration & Safety Runbook

## 1. Safety Principles & Zero-Brick Protocols

1. **Host-Only Building:** All rootfs images, kernel binaries, and initramfs archives are compiled exclusively on the Fedora host machine (`~/Documents/Projects/Apps/a70/native-linux/`).
2. **No Unapproved Flashing:** No partition table rewrite, `dd` write to raw flash, or flashing command will ever be issued without separate, explicit user authorization.
3. **Full System Backup Available:** The complete Hermes and 9Router setup is backed up to `/home/jonathan/Backups/A70-Linux-Migration/`.

---

## 2. Emergency Recovery Modes

Samsung devices have robust low-level hardware recovery paths:

### 1. Download Mode (Odin / Heimdall Mode)
- **How to Enter:**
  1. Power off device completely.
  2. Hold `Volume Up` + `Volume Down` simultaneously.
  3. Insert USB-C cable connected to Fedora laptop.
  4. Blue screen appears; press `Volume Up` once to confirm Download Mode.
- **Tools on Fedora:**
  - `heimdall detect` — Verifies USB connection to bootloader.
  - `heimdall flash --BOOT <original_boot.img>` — Restores original Android boot partition.
  - `heimdall flash --RECOVERY <original_recovery.img>` — Restores original recovery partition.

### 2. Recovery Mode (TWRP / OrangeFox)
- **How to Enter:**
  1. Power off device.
  2. Connect USB cable to PC or charger.
  3. Hold `Power` + `Volume Up` until Samsung logo appears, then release `Power`.
- **Capabilities:**
  - Full ADB root shell access (`adb shell`).
  - Access to `e2fsck`, `resize2fs`, `parted`, and file restoration.
  - Backup & restore of `boot`, `dtbo`, `system`, `vendor`, and `userdata`.

---

## 3. Step-by-Step Restoration Procedures

### Scenario A: Linux Boot Image Hangs on Splash Screen
- **Symptom:** Phone vibrates, displays Samsung logo, and remains frozen.
- **Recovery:**
  1. Hold `Power` + `Volume Down` for 7 seconds to force reboot.
  2. Device reboots naturally into standard crDroid Android 13 (because the primary `sda21` boot partition is unmodified).
  3. No flash or recovery required!

### Scenario B: Restoring Original Recovery Partition
If Debian kernel was flashed to `sda22` (`recovery`) and you want to restore the stock Android recovery:
```bash
# On Fedora host:
adb reboot download
heimdall flash --RECOVERY /home/jonathan/Backups/A70-Linux-Migration/stock_recovery.img
```
Or via ADB root inside Android:
```bash
adb root
adb push stock_recovery.img /data/local/tmp/
adb shell "dd if=/data/local/tmp/stock_recovery.img of=/dev/block/bootdevice/by-name/recovery bs=4096"
```

### Scenario C: Rolling Back Userdata Filesystem
If `sda29` was shrunk and needs to be expanded back to full disk capacity:
1. Boot into TWRP recovery.
2. In ADB shell:
   ```bash
   parted /dev/block/sda rm 30
   parted /dev/block/sda resizepart 29 100%
   e2fsck -f /dev/block/sda29
   resize2fs /dev/block/sda29
   ```
3. Reboot to Android. Userdata is restored to its original 116.57 GiB capacity with zero data loss.
