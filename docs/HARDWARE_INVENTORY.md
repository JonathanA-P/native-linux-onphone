# Samsung Galaxy A70 (SM-A705F / a70q) — Hardware Inventory

## 1. Device Overview

- **Device Name:** Samsung Galaxy A70
- **Model:** SM-A705F / DS
- **Codename:** a70q
- **Board Hardware Name:** SRPRL06C005 / sm6150
- **SoC:** Qualcomm Snapdragon 675 (SM6150 / SM6150P)
- **CPU Architecture:** 64-bit ARMv8-A (aarch64)
  - 2x Kryo 460 Gold (ARM Cortex-A76 @ up to 2.02 GHz)
  - 6x Kryo 460 Silver (ARM Cortex-A55 @ up to 1.71 GHz)
- **GPU:** Qualcomm Adreno 612 (OpenGL ES 3.2, Vulkan 1.1, OpenCL 2.0)
- **RAM:** 6.0 GB LPDDR4X (usable ~5.4 GB, rest reserved for modem/trustzone/cma)
- **Internal Storage:** 128 GB UFS 2.1 (KLUDG4U1EA-B0C1 / Micron, partitioned under `/dev/block/sda`, `sdb`, `sdc`, `sdd`, `sde`, `sdf`)
- **Current Android System:** crDroid 13 (Android 13, LineageOS 20-derived)
- **Baseline Kernel:** Linux 4.14.190-perf+ (aarch64)

---

## 2. Display Subsystem (EVIDENCE-BASED STATUS)

> [!IMPORTANT]
> **Strict Hardware Reality:**
> Physical inspection and sysfs analysis confirm that this device is equipped with an **aftermarket non-OLED replacement display**.
> - **Display Technology:** UNKNOWN. The replacement assembly is non-OLED, but the exact panel technology (e.g. Incell, TFT, IPS) and specific panel controller IC are unconfirmed by hardware markers.
> - **Backlight Control:** Verified driven via Qualcomm PM6150L WLED driver at `/sys/class/backlight/backlight` (Max brightness: `4095`).
> - **Android 13 Status:** Operational under crDroid 13 via Qualcomm `sde_kms` / `msm_drm` DSI command/video mode.
> - **Native Linux Status:** **UNVERIFIED**. Whether the compiled Linux 4.14 DRM KMS stack properly initializes the panel power sequencing and MIPI-DSI timings without Android surfaceflinger must be proven in live native boot.

---

## 3. Touchscreen Subsystem (EVIDENCE-BASED STATUS)

- **Controller IC:** Imagis IST40XX (`ist40xx-ts`)
- **Kernel Device Node:** `sec_touchscreen`
- **Input Node on Android:** `/dev/input/event2`
- **Bus Interface:** I2C bus `884000.i2c/i2c-1` @ address `0x50` (`touchscreen@50`)
- **Capabilities on Android 13:** Multi-touch (up to 10 points), coordinates `(0,0)` to `(1080, 2400)`.
- **Native Linux Status:** **UNVERIFIED**. While `CONFIG_TOUCHSCREEN_IST40XX=y` is compiled into the kernel, true event generation under pure Linux `libinput` depends on panel power-on sequencing and interrupt delivery during native boot.

---

## 4. Power Management & Battery

- **PMIC:** Qualcomm PM6150 + PM6150L
- **Battery Management:** Samsung AFC (Adaptive Fast Charging) + QC 3.0 / USB-PD
  - Drivers: `drivers/battery_qc/sec_battery_sysfs_qc.c`, `drivers/ccic/samsung_usbpd.c`, `drivers/afc/pm6150-afc.c`
  - Power Supply Nodes: `/sys/class/power_supply/battery`, `/sys/class/power_supply/usb`
- **Native Linux Status:** PARTIAL (sysfs battery stats exposed; AFC fast charging handled in-tree).

---

## 5. Storage & Partition Topology

- **Primary Storage:** UFS 2.1 LUN 0 (`/dev/block/sda`, ~119.24 GiB total)
  - `sda20`: `dtbo` (10 MiB) — Device Tree Overlays
  - `sda21`: `boot` (64 MiB) — Primary Android Boot partition
  - `sda22`: `recovery` (79 MiB / 80,884 KiB) — Android Recovery partition (Staging target for Linux boot)
  - `sda23`: `system` (5.5 GiB) — Android System partition
  - `sda24`: `vendor` (1.0 GiB) — Android Vendor partition
  - `sda29`: `userdata` (116.57 GiB total, ~88 GiB currently free)
- **Native Linux Status:** PASS (In-tree `ufshcd-qcom` driver is monolithic `=y`, handles ext4/f2fs partitions natively).

---

## 6. USB & Connectivity

- **USB:** Qualcomm DWC3 dual-role controller (`a600000.dwc3`)
  - Gadget mode: USB ConfigFS supporting RNDIS (USB Ethernet), ADB, Serial ACM
  - Native Linux Status: PASS (In-tree ConfigFS gadget initialized in early initramfs with static IP `192.168.42.2`).
- **Wi-Fi & Bluetooth:** Qualcomm WCN3990 (Cherokee) combo chip
  - Native Linux Status: NOT TESTED (Requires firmware loading from `/vendor/firmware/`).
- **Audio:** Qualcomm WCD937x codec + QDSP6
  - Native Linux Status: NOT TESTED (Requires ALSA UCM profiles or Audio HAL).
- **Cellular Modem:** Qualcomm Snapdragon X12 LTE Modem
  - Native Linux Status: NOT TESTED (Requires QRTR daemon and ModemManager/oFono).
