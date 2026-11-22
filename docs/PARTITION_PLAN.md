# Samsung Galaxy A70 — Partition & Storage Allocation Plan

## 1. Storage Overview

The device contains 128 GB UFS 2.1 flash memory. The main user storage resides on LUN 0 (`/dev/block/sda`), formatted with a standard GPT partition table.

### Current Partition Table (Key Partitions on `sda`):

| Partition | Block Dev | Label | Start LBA | End LBA | Sectors (4096B) | Size (MiB/GiB) | Filesystem |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| 20 | `sda20` | `dtbo` | 33,280 | 35,839 | 2,560 | 10 MiB | Raw DTBO |
| 21 | `sda21` | `boot` | 35,840 | 52,223 | 16,384 | 64 MiB | Android Boot v1 |
| 22 | `sda22` | `recovery` | 52,224 | 72,444 | 20,221 | ~79 MiB | Android Recovery v1 |
| 23 | `sda23` | `system` | 72,448 | 1,514,239 | 1,441,792 | 5,632 MiB | ext4 (Android System) |
| 24 | `sda24` | `vendor` | 1,514,240 | 1,776,383 | 262,144 | 1,024 MiB | ext4 (Android Vendor) |
| ... | ... | ... | ... | ... | ... | ... | ... |
| 29 | `sda29` | `userdata` | 2,169,856 | 32,693,243 | 30,523,388 | ~116.57 GiB | ext4 / f2fs |

*Current `userdata` usage:*
- Total capacity: ~116.57 GiB
- Used space: ~28.0 GiB (crDroid + user data + Hermes backup files)
- Free space: ~88.5 GiB

---

## 2. Proposed Dual-Boot Target Partition Layout

> [!WARNING]
> **SAFETY DIRECTIVE:**
> This plan is strictly analytical and prepared for future execution. No partition table modifications or filesystem resize commands will be executed without separate, explicit user approval.

### Phase 1: Safe Non-Destructive Loopback Testing (Zero Partition Risk)
Before touching the GPT partition table:
1. Place `debian-rootfs.img` (56 GiB sparse ext4 image file) directly inside `/data/debian-rootfs.img`.
2. The initramfs mounts `/data` (`sda29`), configures `/dev/loop0` on `/data/debian-rootfs.img`, and performs `switch_root` to `/dev/loop0`.
3. **Risk level:** Absolutely ZERO partition danger. The GPT table is untouched. If anything fails, simply delete the file `/data/debian-rootfs.img` from Android.

### Phase 2: Dedicated Physical Partitioning (True Dual Boot)
When physical repartitioning is approved:
1. **Shrink `sda29` (`userdata`):**
   - Shrink filesystem from 116.57 GiB down to 58.0 GiB using `resize2fs -p /dev/block/sda29 58G` in TWRP/Recovery mode.
   - Adjust GPT partition 29 end sector to sector `17,431,551` (~58 GiB).
2. **Create `sda30` (`debian`):**
   - Start sector: `17,431,552`
   - End sector: `32,693,243`
   - Size: ~58.5 GiB
   - Label: `DEBIAN_ROOT`
   - Format: `mkfs.ext4 -L DEBIAN_ROOT /dev/block/sda30`
3. **Partition Allocation Result:**
   - Android Userdata (`sda29`): ~58 GiB (plenty of room for apps and storage)
   - Debian Rootfs (`sda30`): ~58.5 GiB (spacious environment for Debian 13, Phosh, development packages, and containers)
