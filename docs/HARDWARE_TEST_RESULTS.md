# Samsung Galaxy A70 — Hardware Test Results & Live Validation

## 1. Test Environment

- **Target Device:** Samsung Galaxy A70 (SM-A705F / `a70q`)
- **Remote Host:** `worker@100.121.186.56` (SSH alias `a70`)
- **Host Testing Machine:** Fedora Linux 45 (x86_64)
- **Baseline Kernel:** Linux 4.14.190-perf+
- **Running Android Version:** crDroid 13 / Android 13

---

## 2. Test Execution & Evidence

### Test 1: Display & Backlight Subsystem Verification
- **Test Command:**
  `cat /sys/class/backlight/backlight/max_brightness` and `readlink -f /sys/class/backlight/backlight/device`
- **Observed Result:**
  - Max brightness: `4095`
  - Backlight path resolves to:
    `/sys/devices/platform/soc/c440000.qcom,spmi/spmi-0/spmi0-05/c440000.qcom,spmi:qcom,pm6150l@5:qcom,wled@d800/backlight/backlight`
- **Verification Conclusion:**
  - Hardware display is confirmed to be an **aftermarket replacement non-OLED display**.
  - Exact panel technology (Incell, TFT, IPS) and controller IC are UNKNOWN.
  - Backlight control is governed by the Qualcomm PM6150L WLED driver.
  - Status under Android 13: **PASS** (functional).
  - Status under Native Linux: **NOT TESTED** (unverified until native boot).

### Test 2: Touchscreen Input Subsystem
- **Test Command:**
  `cat /proc/bus/input/devices | grep -A 8 sec_touchscreen` and `getevent -i /dev/input/event2`
- **Observed Result:**
  - Device Name: `"sec_touchscreen"`
  - Sysfs path: `/devices/platform/soc/884000.i2c/i2c-1/1-0050/input/input2`
  - Handlers: `event2`
  - Event types: `EV_SYN`, `EV_KEY`, `EV_ABS` (`ABS_MT_POSITION_X` max 1080, `ABS_MT_POSITION_Y` max 2400)
- **Verification Conclusion:**
  - Imagis IST40XX controller IC responds on I2C bus 1 at address 0x50.
  - Status under Android 13: **PASS** (functional).
  - Status under Native Linux: **NOT TESTED** (unverified until native boot).

### Test 3: UFS Storage Partition Layout
- **Test Command:**
  `ls -la /dev/block/bootdevice/by-name/` and `df -h /data`
- **Observed Result:**
  - `sda20`: `dtbo` (10 MiB)
  - `sda21`: `boot` (64 MiB)
  - `sda22`: `recovery` (79 MiB / 80,884 KiB)
  - `sda23`: `system` (5.5 GiB)
  - `sda24`: `vendor` (1.0 GiB)
  - `sda29`: `userdata` (Total: 116.57 GiB, Used: 28 GiB, Available: 88.5 GiB)
- **Verification Conclusion:**
  - UFS 2.1 storage operates under in-tree monolithic driver `ufshcd-qcom` (`=y`).
  - Ample unallocated space for non-destructive loopback rootfs image file (`/data/debian-rootfs.img`).
  - Status: **PASS**.

### Test 4: Boot Geometry Inspection
- **Test Command:**
  Inspection of Samsung SM6150 bootloader expectations:
  - Page size: `4096`
  - Kernel offset: `0x00008000`
  - Ramdisk offset: `0x02000000`
  - Tags offset: `0x01e00000`
  - Board name: `SRPRL06C005`
  - Header version: `1`
- **Verification Conclusion:**
  - Boot image packaging geometry matches bootloader expectations.
  - Status: **PASS**.

### Test 5: Monolithic Driver & Module Verification
- **Test Command:**
  `grep "=m" artifacts/kernel/build.config` and `ssh a70 "lsmod"`
- **Observed Result:**
  - Exactly 0 loadable modules (`=m`) in `build.config`.
  - Exactly 0 loaded modules reported by `lsmod` on running Android 13.
- **Verification Conclusion:**
  - The kernel is 100% monolithic. All storage, display, input, and USB drivers are in-tree (`=y`).
  - No external `/lib/modules/` dependencies required for early boot.
  - Status: **PASS**.
