# Samsung Galaxy A70 (SM-A705F / a70q) — Actual Partition Inventory

**Verification Date:** 2026-11-20  
**Verification Method:** Direct `adb shell` kernel `/proc/partitions`, sysfs, and `ls -la /dev/block/bootdevice/by-name/` on connected device `RR8M50J37GY`.  
**Storage Architecture:** UFS 2.1 Multi-LUN (`sda`, `sdb`, `sdc`, `sdd`).

---

## 1. Primary User LUN 0 (`/dev/block/sda` — 124,874,752 KB / ~128 GB)

| Partition Node | By-Name Label | Size (KB / MB) | Filesystem / Format | Mount Point on Android | Encryption / Security |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `sda1` | `modemst1` | 2,048 KB (2 MB) | Raw NVRAM | N/A (Modem NV) | Protected |
| `sda2` | `modemst2` | 2,048 KB (2 MB) | Raw NVRAM | N/A (Modem NV) | Protected |
| `sda3` | `fsc` | 4 KB | Raw | N/A | Protected |
| `sda4` | `ssd` | 8 KB | Raw | N/A | Protected |
| `sda5` | `persist` | 32,768 KB (32 MB) | ext4 | `/mnt/vendor/persist` | SELinux labeled |
| `sda6` | `efs` | 20,480 KB (20 MB) | ext4 | `/mnt/vendor/efs` | Critical EFS/IMEI |
| `sda7` | `param` | 12,288 KB (12 MB) | Raw | N/A (Boot parameters) | Protected |
| `sda8` | `debug` | 10,240 KB (10 MB) | Raw | N/A | Debug |
| `sda9` | `sec_efs` | 20,480 KB (20 MB) | ext4 | `/efs` | Critical Knox/EFS |
| `sda10` | `misc` | 1,024 KB (1 MB) | Raw | N/A (BCB command) | Bootloader control |
| `sda11` | `keystore` | 512 KB | Raw | N/A | Keystore |
| `sda12` | `bota` | 40,960 KB (40 MB) | Raw | N/A | Firmware OTA |
| `sda13` | `fota` | 21,504 KB (21 MB) | Raw | N/A | Firmware OTA |
| `sda14` | `persistent` | 512 KB | Raw | N/A (FRP) | Factory Reset Protect |
| `sda15` | `steady` | 4,096 KB (4 MB) | Raw | N/A | Samsung Steady |
| `sda16` | `dqmdbg` | 16,384 KB (16 MB) | Raw | N/A | Diagnostic |
| `sda17` | `apnhlos` | 97,280 KB (95 MB) | vfat | `/vendor/firmware_mnt` | Read-only DSP/firmware |
| `sda18` | `modem` | 87,040 KB (85 MB) | vfat | `/vendor/firmware-modem` | Read-only baseband |
| `sda19` | `dsp` | 32,768 KB (32 MB) | ext4 | `/vendor/dsp` | Read-only Hexagon DSP |
| **`sda20`** | **`dtbo`** | **10,240 KB (10 MB)** | **Raw DTBO v0** | N/A (Loaded by S-Boot) | Device Tree Overlays |
| **`sda21`** | **`boot`** | **65,536 KB (64 MB)** | **Android Boot v1** | N/A (Primary Android kernel) | crDroid 13 Android Kernel |
| **`sda22`** | **`recovery`** | **80,884 KB (~79 MB)**| **Android Recovery v1**| N/A (Dual-Boot Carrier) | Target for Debian Boot Carrier |
| `sda23` | `system` | 5,632,000 KB (5.5 GB)| ext4 | `/` (Android System) | System-as-root |
| `sda24` | `vendor` | 1,024,000 KB (1.0 GB)| ext4 | `/vendor` | Android HALs & Blobs |
| `sda25` | `product` | 614,400 KB (600 MB) | ext4 | `/product` | Android Product apps |
| `sda26` | `cache` | 409,600 KB (400 MB) | ext4 | `/cache` | Recovery & OTA cache |
| `sda27` | `hidden` | 10,240 KB (10 MB) | ext4 | N/A | Carrier preload |
| `sda28` | `omr` / `metadata` | 51,200 KB (50 MB) | ext4 | `/metadata` | FBE encryption metadata |
| **`sda29`** | **`userdata`** | **116,573,020 KB (~111.17 GB)** | **F2FS** | **`/data`** | **File-Based Encryption (FBE)** |

---

## 2. Auxiliary LUNs

### LUN 1 (`sdb` — 8,192 KB / 8 MB)
- `sdb1`: `xbl` (4,096 KB) — Qualcomm eXtensible Boot Loader (Primary bootloader stage)
- `sdb2`: `xbl_config` (3,940 KB) — XBL configuration

### LUN 2 (`sdc` — 8,192 KB / 8 MB)
- `sdc1`: `xbl_bak` (4,096 KB) — XBL backup copy
- `sdc2`: `xbl_config_bak` (3,940 KB) — XBL config backup

### LUN 3 (`sdd` — 61,440 KB / 60 MB)
- `sdd19`: `vbmeta` (64 KB) — Android Verified Boot metadata
- `sdd5`: `abl` (4,096 KB) — Android Bootloader (S-Boot / Fastboot)
- `sdd2`: `tz` (4,096 KB) — Qualcomm TrustZone
- `sdd9`: `keymaster` (1,024 KB) — Keystore/Keymaster HAL
- `sdd10` / `sdd11`: `cmnlib` / `cmnlib64` (512 KB / 512 KB) — Common 32/64-bit cryptographic libraries

---

## 3. Critical Encryption & Dual-Boot Constraints

1. **Userdata Filesystem Type:** `/dev/block/sda29` is formatted as **F2FS** (Flash-Friendly File System), NOT ext4.
2. **Encryption:** `ro.crypto.state=encrypted`, `ro.crypto.type=file`. Inodes in `sda29` are individually encrypted using Linux kernel fscrypt under Android Keymaster keys stored in TrustZone (`sdd2`/`sdd9`).
3. **Loopback Feasibility:** A pure Linux initramfs WITHOUT Android vold/keystore CANNOT read plaintext files placed inside an encrypted F2FS `/data` partition.
4. **Architectural Consequence:** True native dual-boot requires either a dedicated physical partition (e.g. `sda30` ext4) OR formatting a dedicated storage area that does not depend on Android FBE keystore.
