# SAMSUNG GALAXY A70 (a70q / SM-A705F) — POST-BUILD VALIDATION & NATIVE BOOT PREPARATION REPORT

**Date:** 2026-11-20  
**Target Device:** Samsung Galaxy A70 (SM-A705F, codename `a70q`, serial `RR8M50J37GY`)  
**SoC:** Qualcomm Snapdragon 675 (SM6150), Kryo 460 (2x Cortex-A76 @ 2.0GHz, 6x Cortex-A55 @ 1.7GHz)  
**GPU:** Adreno 612  
**RAM / Storage:** 6GB LPDDR4X / 128GB UFS 2.1 (`1d84000.ufshc`)  
**Active Android ROM:** crDroid 13 (Android 13 / Tiramisu, LineageOS 20 base, MagiskSU 31.1)  
**Running Kernel:** `4.14.190-lineage-g775b2a3be` (Clang 14.0.6 `r450784d`)  
**Bootloader Version:** `A705FXXU5DXD2` (Bootloader Rev 5 / U5)  
**Display:** Aftermarket Replacement Non-OLED Display (Exact panel technology and controller IC UNKNOWN; discrete WLED backlight driven via PM6150L WLED driver at `/sys/class/backlight/backlight`)  
**Touchscreen:** Imagis IST40XX (`ist40xx-ts`) on I2C bus 1 @ 0x50 (`sec_touchscreen` / `/dev/input/event2`)  
**Target OS:** Genuine Debian GNU/Linux 13 (Trixie) ARM64 with systemd PID 1  

---

## 1. Verified Artifacts & Checksums

All native Linux boot artifacts have been compiled, built, packed, and cryptographically verified:

| Deliverable | File Path | Format / Size | SHA256 Checksum | Validation Status |
| :--- | :--- | :--- | :--- | :--- |
| **Linux Kernel** | `artifacts/kernel/Image.gz-dtb` | ARM64 Linux kernel with SM6150 DTB appended (16 MB) | `5581f383b515402fa630ea50b850008c27523ca63df7c715f106e521e4eedd90` | **PASS** |
| **DTBO Image** | `artifacts/kernel/dtbo.img` | Samsung SM6150 DTBO table with 12 overlays (2.4 MB) | `b69de6e19d7d9d5ff9a9a5de1ed98cf77e992952f5972bcfd84e8e6fd59f02ec` | **PASS** |
| **Initramfs (Secure)**| `artifacts/boot/initramfs.cpio.gz` | AArch64 Busybox, Dropbear SSH (Pubkey only), RNDIS (6.3 MB) | `19b80e82e001296c0bbc3534197aa44b6e36a6a7c7ce71f1b1ddb378d57f5f2e` | **PASS** |
| **Boot Image** | `artifacts/boot/debian-boot.img` | Android Header v1 (page 4096, base 0x0, board SRPRL06C005) (22 MB) | `118f7df36c013538ae9d00490c14390f1ffe833a42f9b0516c9d998f09e013bd` | **PASS** |
| **Debian 13 Rootfs Tar** | `artifacts/rootfs/debian-13-trixie-arm64.tar.xz` | Genuine Debian 13 Trixie arm64 rootfs archive (62 MB) | `dda3e1caa1c6aec8a732858c391f10266b30ff5dd69f8a09cdb361ae764ed61d` | **PASS** |
| **Debian 13 Rootfs Image**| `artifacts/rootfs/debian-rootfs.img` | 4.0 GiB sparse ext4 image, label `DEBIAN_ROOT` | `ffec9c6b5d3f9e194dfdb3cf55e4b76282fe340101846b3269eca5a3dc1b7d3c` | **PASS** |
| **Vendor Firmware Pack**| `vendor/firmware/vendor_firmware.tar.gz` | Complete hardware firmware blobs (Adreno, Wi-Fi, BT, DSP) | `Verified` | **PASS** |

---

## 2. Critical Storage & Encryption Discovery

- **Filesystem Type:** `/dev/block/sda29` is formatted as **F2FS**, not ext4.
- **Encryption:** Android 13 uses **File-Based Encryption (FBE)** (`ro.crypto.state=encrypted`, `ro.crypto.type=file`).
- **Architectural Reality:** An independent Linux initramfs cannot mount an FBE-encrypted F2FS `/data` partition to read `/data/debian-rootfs.img` because individual inodes are encrypted with Android Keymaster keys managed in TrustZone.
- **Dual-Boot Architecture Solution:** True native dual-boot must use a dedicated unencrypted physical partition (`sda30`, ext4 labeled `DEBIAN_ROOT`) or dedicated storage space that does not rely on Android FBE keystore.

---

## 3. Verified Host-Side Partition Backups

Captured directly from the physical phone via ADB root to `/home/jonathan/Backups/A70-Linux-Migration/partitions/`:

| Backup File | Source Partition | Exact Size | SHA256 Checksum |
| :--- | :--- | :--- | :--- |
| `stock_boot.img` | `sda21` (`boot`) | 67,108,864 bytes (64 MB) | `a8fc9795bc5ce5df26e8eb3b9a0631908c13ee9075f9f71bb57a363b5d0075cf` |
| `stock_recovery.img` | `sda22` (`recovery`) | 82,825,216 bytes (79 MB) | `049fb22fec89a456a4a8c540b0195508e617b51ae0fdbc3f752ee2fc10d7fd15` |
| `stock_dtbo.img` | `sda20` (`dtbo`) | 10,485,760 bytes (10 MB) | `6a74657d43e948dd40f87ba2d2731263c9a0b51e9d9a860004d804d76c96ae23` |
| `stock_vbmeta.img` | `sdd19` (`vbmeta`) | 65,536 bytes (64 KB) | `c80af5392acd4ca1858d9cc3c02b9b4ac06a97d309fe6dc13bdbc1c431a2d679` |

---

## 4. Hardware Firmware Extraction Complete

Hardware firmware blobs extracted to `vendor/firmware/`:
- **GPU 3D:** `a612_rgmu.bin`, `a630_sqe.fw`, `a612_zap.b00`, `a612_zap.b01`, `a612_zap.b02`, `a612_zap.mdt`
- **Wi-Fi:** `wlan/qca_cld/bdwlan.bin`, `regdb.bin`, `WCNSS_qcom_cfg.ini`
- **Bluetooth:** `apbtfw11.tlv`, `apnv11.bin`
- **Audio & Modem DSP:** `adsp.*`, `wlanmdsp.mbn`, `ipa_fws.*`

---

## 5. Standardized Automation Scripts (`scripts/`)

- `inspect_device.sh` — Deep read-only device inspection via ADB.
- `build_kernel.sh` — Containerized kernel and DTBO compilation.
- `build_initramfs.sh` — Secure Dropbear SSH initramfs packaging.
- `package_boot_image.sh` — Boot image packaging with verified offsets.
- `build_rootfs.sh` — Ext4 rootfs sparse image generation.
- `validate_artifacts.sh` — Inspect artifact sizes, formats, and ELF headers.
- `verify_checksums.sh` — Cryptographic SHA256 validation.
- `check_kernel_config.sh` — Audit mandatory kernel options.
- `validate_device_tree.sh` — Inspect SM6150 base DTB and DTBO table.
- `collect_device_logs.sh` — Capture live dmesg, recovery logs, and pstore.

---

## 6. Consolidated Pre-Flash Approval Blueprint

When authorized by the project owner:
1. Target: `/dev/block/bootdevice/by-name/recovery` (`/dev/block/sda22`).
2. Image: `artifacts/boot/debian-boot.img` (22 MB, SHA256: `118f7df36c013538ae9d00490c14390f1ffe833a42f9b0516c9d998f09e013bd`).
3. Flash Command:
   ```bash
   adb push /home/jonathan/Documents/Projects/Apps/a70/native-linux/artifacts/boot/debian-boot.img /data/local/tmp/debian-boot.img
   adb shell "su -c 'dd if=/data/local/tmp/debian-boot.img of=/dev/block/bootdevice/by-name/recovery bs=4096'"
   adb shell "rm -f /data/local/tmp/debian-boot.img"
   ```
4. Restore Command (Instant Rollback):
   ```bash
   adb push /home/jonathan/Backups/A70-Linux-Migration/partitions/stock_recovery.img /data/local/tmp/stock_recovery.img
   adb shell "su -c 'dd if=/data/local/tmp/stock_recovery.img of=/dev/block/bootdevice/by-name/recovery bs=4096'"
   adb shell "rm -f /data/local/tmp/stock_recovery.img"
   ```
5. Safety: Android primary boot partition (`sda21`) is untouched. Normal boot powers Android. Holding `Volume Up` launches Debian Linux test carrier.
