# Samsung Galaxy A70 (a70q / SM-A705F) — First Native Linux Boot & Bring-Up Guide

**Document Purpose:** Complete operational guide for the first native Linux diagnostic boot milestone on Samsung Galaxy A70.  
**Hardware Serial:** `RR8M50J37GY`  
**Host Workstation:** Fedora Linux 45 (`192.168.42.1/24`)

---

## 1. Architectural Strategy: Diagnostic First Boot

Before deploying a full desktop rootfs or modifying internal partitions, we perform a **Diagnostic First Boot**:
- **Zero Rootfs Dependency:** Operates 100% inside `initramfs-diagnostic.cpio.gz`. Does not require or touch Android `/data` (avoiding F2FS / FBE encryption complications).
- **Zero Unauthenticated Network Exposure:** Runs Dropbear SSH on `192.168.42.2:22` restricted strictly to the Fedora host's Ed25519 public key.
- **Hardware Validation:** Initializes Qualcomm PM6150L WLED backlight, USB DWC3 ConfigFS gadget, and logs hardware state directly to `/var/log/native_boot.log`.
- **Carrier Partition:** Staged to `sda22` (`recovery`, 79 MiB). The primary Android boot partition (`sda21`) is **unmodified**.

---

## 2. Verified Deliverables & Artifacts

All files are located in `native-linux/artifacts/boot/`:

| Artifact | Path | Size | SHA256 Checksum |
| :--- | :--- | :--- | :--- |
| **Diagnostic Initramfs** | `artifacts/boot/initramfs-diagnostic.cpio.gz` | 6.3 MB | `7172fff058fdff86cb5f408f225abec285843aeb101681e3b7d0525b991d5998` |
| **Diagnostic Boot Image** | `artifacts/boot/a70-diagnostic-boot.img` | 22 MB | `198a2a079e0bfbb1a3572933c10f7c3a030d4f0e9de039f56bb273dd5815f24b` |
| **Kernel Binary** | `artifacts/kernel/Image.gz-dtb` | 16 MB | `5581f383b515402fa630ea50b850008c27523ca63df7c715f106e521e4eedd90` |
| **DTBO Overlays** | `artifacts/kernel/dtbo.img` | 2.4 MB | `b69de6e19d7d9d5ff9a9a5de1ed98cf77e992952f5972bcfd84e8e6fd59f02ec` |

---

## 3. Host-Side Diagnostic Scripts

1. **Connect & Verify Diagnostic Link:**
   ```bash
   /home/jonathan/Documents/Projects/Apps/a70/native-linux/scripts/connect_diagnostic_device.sh
   ```
   *Action:* Detects USB RNDIS interface, assigns `192.168.42.1/24`, tests ping to `192.168.42.2`, and opens authenticated SSH shell.

2. **Automated Log Harvester:**
   ```bash
   /home/jonathan/Documents/Projects/Apps/a70/native-linux/scripts/collect_native_boot_logs.sh
   ```
   *Action:* Gathers `dmesg`, `/proc/cmdline`, `/proc/partitions`, input devices, backlight, thermal, and power nodes into `logs/native-boot/boot_<timestamp>/`.

---

## 4. Boot Selection & Key Combinations on Samsung A70

Samsung bootloader (S-Boot / XBL) checks hardware keys at power-on:

1. **Normal Power On (Tap Power Button):**
   - Bootloader executes `sda21` (`boot`).
   - crDroid Android 13 starts normally. Zero impact.
2. **Recovery Boot (USB Cable Plugged In + Hold `Power` + `Volume Up`):**
   - Bootloader executes `sda22` (`recovery`).
   - Native Linux Diagnostic kernel starts.
   - Screen turns on (PM6150L WLED backlight).
   - USB RNDIS gadget initializes on Fedora host (`192.168.42.2`).
3. **Emergency Hard Reset (Kernel Freeze or Black Screen):**
   - Hold `Power` + `Volume Down` for **7 seconds**.
   - Hardware PMIC forcibly resets the phone.
   - Device boots back into standard Android crDroid 13.

---

## 5. Consolidated Pre-Flash Staging Blueprint

When the owner authorizes flashing to the recovery partition:

### Staging Command (ADB Root):
```bash
adb push /home/jonathan/Documents/Projects/Apps/a70/native-linux/artifacts/boot/a70-diagnostic-boot.img /data/local/tmp/
adb shell "su -c 'dd if=/data/local/tmp/a70-diagnostic-boot.img of=/dev/block/bootdevice/by-name/recovery bs=4096'"
adb shell "rm -f /data/local/tmp/a70-diagnostic-boot.img"
```

### Verified Instant Rollback Command:
```bash
adb push /home/jonathan/Backups/A70-Linux-Migration/partitions/stock_recovery.img /data/local/tmp/
adb shell "su -c 'dd if=/data/local/tmp/stock_recovery.img of=/dev/block/bootdevice/by-name/recovery bs=4096'"
adb shell "rm -f /data/local/tmp/stock_recovery.img"
```
*(Backup `stock_recovery.img` verified with SHA256 `049fb22fec89a456a4a8c540b0195508e617b51ae0fdbc3f752ee2fc10d7fd15` on Fedora).*
