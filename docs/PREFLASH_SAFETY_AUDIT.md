# Samsung Galaxy A70 (SM-A705F) — Pre-Flash Safety Audit & Authorization Blueprint

**Audit Date:** 2026-11-20  
**Device Serial:** `RR8M50J37GY`  
**Host Workstation:** Fedora Linux 45 (x86_64)

---

## 1. Safety Audit Findings

### Physical Partition Verification
- `sda21` (`boot`): 64 MB (67,108,864 bytes). Holds crDroid 13 Android kernel. **MUST NOT BE FLASHED.**
- `sda22` (`recovery`): 79 MB (82,825,216 bytes). Standard Android Recovery carrier. **DESIGNATED LINUX CARRIER TARGET.**
- `sda29` (`userdata`): 111.17 GB (116,573,020 KB). Formatted as **F2FS** with **FBE (File-Based Encryption)**.
  - *Finding:* Because userdata is encrypted with Android Keymaster keys, a pure Linux initramfs cannot mount `/data` and open `/data/debian-rootfs.img` without Android userspace.
  - *Safety Ruling:* We must NOT attempt `resize2fs` on `sda29` (as it is F2FS, not ext4). Any future partition shrinking must be planned with `resize.f2fs` in TWRP/Recovery after full user backup.

---

## 2. Verified Host-Side Partition Backups

The following backups have been captured from the physical device over ADB root and cryptographically verified on the Fedora host under `/home/jonathan/Backups/A70-Linux-Migration/partitions/`:

| Backup File | Source Partition | Exact Size | SHA256 Checksum | Verification Status |
| :--- | :--- | :--- | :--- | :--- |
| `stock_boot.img` | `/dev/block/sda21` (`boot`) | 67,108,864 bytes (64 MB) | `a8fc9795bc5ce5df26e8eb3b9a0631908c13ee9075f9f71bb57a363b5d0075cf` | **VERIFIED** |
| `stock_recovery.img` | `/dev/block/sda22` (`recovery`) | 82,825,216 bytes (79 MB) | `049fb22fec89a456a4a8c540b0195508e617b51ae0fdbc3f752ee2fc10d7fd15` | **VERIFIED** |
| `stock_dtbo.img` | `/dev/block/sda20` (`dtbo`) | 10,485,760 bytes (10 MB) | `6a74657d43e948dd40f87ba2d2731263c9a0b51e9d9a860004d804d76c96ae23` | **VERIFIED** |
| `stock_vbmeta.img` | `/dev/block/sdd19` (`vbmeta`) | 65,536 bytes (64 KB) | `c80af5392acd4ca1858d9cc3c02b9b4ac06a97d309fe6dc13bdbc1c431a2d679` | **VERIFIED** |

---

## 3. Consolidated Pre-Flash Approval Blueprint

When the owner decides to execute the live native boot test, this single consolidated operation must be authorized:

1. **Exact Operation:** Write compiled `debian-boot.img` into the recovery carrier partition.
2. **Exact Target Partition:** `/dev/block/bootdevice/by-name/recovery` (`/dev/block/sda22`).
3. **Exact Target Size:** 82,825,216 bytes (Partition capacity: 79 MB; image size: 22 MB).
4. **Exact Source File:** `/home/jonathan/Documents/Projects/Apps/a70/native-linux/artifacts/boot/debian-boot.img`
5. **Exact Image Checksum:** `118f7df36c013538ae9d00490c14390f1ffe833a42f9b0516c9d998f09e013bd`
6. **Execution Command:**
   ```bash
   adb push /home/jonathan/Documents/Projects/Apps/a70/native-linux/artifacts/boot/debian-boot.img /data/local/tmp/debian-boot.img
   adb shell "su -c 'dd if=/data/local/tmp/debian-boot.img of=/dev/block/bootdevice/by-name/recovery bs=4096'"
   adb shell "rm -f /data/local/tmp/debian-boot.img"
   ```
7. **Verified Restore Command (Instant Recovery):**
   ```bash
   adb push /home/jonathan/Backups/A70-Linux-Migration/partitions/stock_recovery.img /data/local/tmp/stock_recovery.img
   adb shell "su -c 'dd if=/data/local/tmp/stock_recovery.img of=/dev/block/bootdevice/by-name/recovery bs=4096'"
   adb shell "rm -f /data/local/tmp/stock_recovery.img"
   ```
8. **Known Risks & Protections:**
   - Primary Android boot partition (`sda21`) is untouched; normal power-on will always boot Android crDroid 13.
   - If the Debian kernel halts or screen remains black, a 7-second PMIC hard reset (`Power` + `Vol Down`) reboots back to Android.
   - The verified `stock_recovery.img` backup can be restored in seconds via ADB or Heimdall.
