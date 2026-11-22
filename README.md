# Native Linux on Phone (Samsung Galaxy A70 — `a70q` / `SM-A705F`)

> [!WARNING]
> ### DISCLAIMER & EXPERIMENTAL STATUS
> **PROCEED AT YOUR OWN RISK.** This project is an advanced, bare-metal research port of Linux onto an Android smartphone platform. Following the guides, flashing partition tables, or executing scripts in this repository carries real risks, including **permanent hardware brick, bootloops, bootloader lockouts, or irreversible data loss**.
> 
> * **NO GUARANTEE OF SUCCESS:** Hardware components on the Samsung Galaxy A70 vary significantly across production batches and regional variants (`SM-A705F`, `SM-A705FN`, `SM-A705GM`, `SM-A705MN`, `SM-A7050`, `SM-A705W`). Crucially, **aftermarket replacement screens** (such as TFT/INCELL panels with non-standard I2C/SPI bridge controllers) behave completely differently from OEM Super AMOLED panels (`S6E3FC2_AMS670TA01_FHD`).
> * **MANUAL ADJUSTMENTS REQUIRED:** Depending on your bootloader version (e.g., Knox S-Boot Rev 1 through Rev 5 / U5), DRAM vendor, and touchscreen digitizer revision, you will need to adapt device tree overlays (DTBO), kernel command-line arguments, and driver timings. This is **not a one-click flash-and-play ROM**.
> * **BACKUP MANDATORY:** Under no circumstances should you proceed without taking full raw block-level backups (`dd` or TWRP/OrangeFox raw partition backups) of `/dev/block/sda`, `boot`, `dtbo`, `recovery`, `efs`, and `userdata`. The authors assume zero liability for any damage to your hardware or loss of data.

---

Genuine bare-metal **Debian GNU/Linux 13 (Trixie)** port for the **Samsung Galaxy A70** (`SM-A705F` / `a70q`), featuring full `systemd` PID 1, custom dual-boot initramfs, **Phosh** Wayland mobile environment, Qualcomm Adreno 612 hardware acceleration via **Turnip Vulkan + Zink**, and headless USB gadget connectivity (CDC-NCM Ethernet + CDC-ACM serial root console).

For the exhaustive trial-by-trial engineering journal, read [docs/EXPERIMENTAL_RESEARCH_LOGS.md](file:///home/jonathan/Documents/Projects/Apps/a70/native-linux/docs/EXPERIMENTAL_RESEARCH_LOGS.md).

---

## Hardware Specifications & Target Platform

| Component | Specification |
| :--- | :--- |
| **Device Model** | Samsung Galaxy A70 (`SM-A705F`, codename `a70q`) |
| **SoC** | Qualcomm Snapdragon 675 (`SM6150`), 8-core Kryo 460 (2x Cortex-A76 @ 2.0GHz, 6x Cortex-A55 @ 1.7GHz) |
| **GPU** | Qualcomm Adreno 612 |
| **Graphics Stack** | Mesa Turnip Vulkan driver + Zink OpenGL ES over Qualcomm downstream KGSL (`/dev/kgsl-3d0`) |
| **Display Panel** | 6.7" FHD+ (1080x2400 @ 60Hz), DSI-1, 2x UI scaling |
| **Touchscreen** | Imagis IST40XX (`sec_touchscreen` / I2C bus 1 @ 0x50) |
| **Storage / Rootfs** | 128GB UFS 2.1 (`/dev/block/sda30` dedicated ext4 rootfs partition, preserving `/dev/block/sda29` Android userdata) |
| **Base Kernel** | Qualcomm downstream Linux 4.14 (`msm-4.14`, LineageOS / crDroid tree adapted for systemd) |
| **Userland** | Debian GNU/Linux 13 (Trixie) ARM64 (`aarch64`) |
| **Compositor / Shell** | Phosh (Pure Wayland Mobile Shell) + Phoc (wlroots) |

---

## Hardware Support & Driver Matrix

| Subsystem | Driver / Interface | Status | Notes |
| :--- | :--- | :---: | :--- |
| **Boot & Init** | Custom C dual-boot initramfs (`dualboot_init.c`) | **Working** | Auto-detects partition layout; non-destructive dual-boot |
| **Display (KMS)** | Qualcomm MSM DRM KMS (`/dev/dri/card0`, DSI-1) | **Working** | Non-atomic modesetting; software cursor forced (`WLR_NO_HARDWARE_CURSORS=1`) |
| **GPU Acceleration** | Mesa Turnip Vulkan + Zink OpenGL | **Working** | Physical Adreno 612 firmware placed; hardware acceleration verified via KGSL |
| **Touchscreen** | Imagis IST40XX (`/dev/input/event*`) | **Working** | Tagged via udev for `seat0` and libinput Wayland mapping |
| **USB CDC-NCM** | Linux ConfigFS Ethernet gadget (`192.168.7.2`) | **Working** | Plug-and-play USB networking to host PC |
| **USB CDC-ACM** | Linux ConfigFS Serial Console (`/dev/ttyGS0` $\rightarrow$ `/dev/ttyACM0`) | **Working** | Direct passwordless root console from host PC |
| **Backlight** | PM6150L WLED (`/sys/class/backlight`) | **Working** | Initialized via `hardware-init` |
| **Battery / PMIC** | PM6150 / SMB1355 charger driver | **Working** | Thermal monitoring and charging functional |
| **Audio** | ALSA / PipeWire / WirePlumber | *WIP* | Qualcomm WCD937x audio codec driver in progress |
| **Wi-Fi / WLAN** | Qualcomm `qcacld-3.0` | *WIP* | Kernel module loads; panic guard in initramfs |

---

## The Engineering Journey: Trials, Failures & Breakthroughs

Bringing desktop Linux onto a modern Qualcomm Snapdragon smartphone is not a simple userland chroot. It requires navigating kernel constraints, driver omissions, and bootloader quirks:

### 1. The Partition Boundary & Non-Destructive Dual-Boot
* **Trial:** Installing native Linux without erasing the user's primary Android ROM (crDroid 13 / LineageOS 20).
* **Breakthrough:** Discovered an unallocated 77GB LUN partition on `/dev/block/sda30`. Formatted it as pure `ext4`, leaving `/dev/block/sda29` (crDroid F2FS userdata) 100% untouched.

### 2. Samsung S-Boot DTBO Validation Header
* **Trial:** Flashing generic `boot.img` resulted in immediate bootloops.
* **Root Cause:** Samsung Knox S-Boot Rev 5 (`A705FXXU5DXD2`) verifies the device tree overlay table (`dtbo.img`, magic `0x1eabb7d7`). Missing matching overlays for SM6150 revision 13 halts bootloader handover.
* **Breakthrough:** Extracted and injected the LineageOS DTBO table (`2,499,651` bytes) with relative offset calculation (`0x1d8e000`).

### 3. Early UART MMIO Bus Fault
* **Trial:** Setting `earlycon=msm_geni_serial,0x880000` halted boot instantly.
* **Root Cause:** Qualcomm SM6150 GENI serial clocks are disabled prior to kernel clock provider initialization. Accessing unclocked MMIO registers triggers ARM64 EL1 hardware faults.
* **Fix:** Stripped `earlycon`, passed `console=null nokaslr printk.devkmsg=on`.

### 4. Qualcomm Wi-Fi Driver Kernel Panic (`qcacld-3.0`)
* **Trial:** Triggering `echo 1 > /sys/kernel/boot_wlan/boot_wlan` on early boot.
* **Root Cause:** Vendor driver `qcacld-3.0` was compiled with `+PANIC_ON_BUG`. Triggering WLAN before regulatory domains and firmware are loaded by userspace caused fatal kernel panic and EDL crash.
* **Fix:** Deferred and isolated WLAN initialization out of the critical boot path.

### 5. Adreno 612 GPU Acceleration over KGSL (Turnip + Zink)
* **Trial:** Bypassing `llvmpipe` CPU software rendering without upstream DRM MSM.
* **Breakthrough:** Utilized Mesa Turnip's native KGSL backend (`TU_DEBUG=kgsl`) communicating directly with `/dev/kgsl-3d0`. Placed physical firmware (`a612_zap.mdt`, `a630_sqe.fw`), adjusted udev permissions (`chmod 666 /dev/kgsl-3d0 /dev/ion`), and layered Zink OpenGL ES (`MESA_LOADER_DRIVER_OVERRIDE=zink`) on top. Confirmed native acceleration: `GPU: Qualcomm Turnip Adreno (TM) 612 [Integrated]`.

### 6. The `seatd` Virtual Terminal Timeout Trap
* **Trial:** Starting Phosh compositor (`phoc`).
* **Root Cause:** `phoc` linked to `libseat`. Because package `seatd` was installed, `libseat` defaulted to `seatd`. But `seatd` strictly requires Linux Virtual Terminals (`/dev/tty0`, `/dev/tty1`, `CONFIG_VT`), which downstream Android kernels disable. `seatd` waited 10s, timed out, and caused `phoc` to crash.
* **Breakthrough:** Completely masked `seatd.service` and forced `LIBSEAT_BACKEND=logind`. `systemd-logind` manages DRM and input directly via DBus without virtual terminals.

### 7. Hardware Cursor Display Blackout
* **Trial:** Mutter / GNOME Shell page flipping.
* **Root Cause:** Qualcomm's downstream KMS driver (`sde_kms`) does not implement hardware cursor planes. Mutter calling KMS `drmModeSetCursor` failed with `-ENXIO`, causing the page flip to fail and triggering display power-off (`dsi_display_disable`).
* **Breakthrough:** Exported `WLR_NO_HARDWARE_CURSORS=1` and `MUTTER_DEBUG_DISABLE_HW_CURSORS=1`. Cursors render in software; panel remains alive.

### 8. Zero-Touch Mobile Adaptation
* **Trial:** User lockout on GDM greeter due to unscaled DPI and missing on-screen keyboard.
* **Breakthrough:** Configured 2x Phoc scaling (`[output:DSI-1] scale = 2`), PAM passwordless authentication, GDM autologin into Phosh, and disabled screen lock via dconf.

---

## Architecture Overview

```
+--------------------------------------------------------------+
|                     Phosh Mobile Shell                       |
|               (Squeekboard On-Screen Keyboard)               |
+--------------------------------------------------------------+
                                |
+--------------------------------------------------------------+
|                    Phoc (wlroots) Compositor                 |
|             (LIBSEAT_BACKEND=logind, scale = 2)              |
+--------------------------------------------------------------+
                                |
        +-----------------------+-----------------------+
        |                                               |
+-------------------------------+       +-------------------------------+
|      Mesa Zink OpenGL ES      |       |      Mesa Turnip Vulkan       |
+-------------------------------+       +-------------------------------+
        |                                               |
+---------------------------------------------------------------+
|                 Qualcomm KGSL Driver (/dev/kgsl-3d0)          |
+---------------------------------------------------------------+
                                |
+---------------------------------------------------------------+
|            Qualcomm MSM DRM Display Controller (DSI-1)        |
+---------------------------------------------------------------+
```

---

## Directory Structure

```
native-linux/
├── artifacts/              # Checksums, build instructions, and verification docs
├── debian/                 # Debian rootfs bootstrapping & image packaging scripts
│   ├── build_rootfs_native_a70.sh
│   └── package_ext4_image.sh
├── device/                 # LineageOS device tree & platform configuration
│   ├── a70q-device/
│   └── sm6150-common/
├── docs/                   # Exhaustive hardware audits and porting documentation
│   ├── EXPERIMENTAL_RESEARCH_LOGS.md   # Complete deep-dive trial journal
│   ├── ACTUAL_PARTITION_INVENTORY.md
│   ├── BUILD_AND_TEST_GUIDE.md
│   ├── DRIVER_COMPATIBILITY_MATRIX.md
│   ├── DUAL_BOOT_ARCHITECTURE.md
│   └── FIRST_NATIVE_BOOT_RESULTS.md
├── initramfs/              # Custom dual-boot initramfs C source and build scripts
│   ├── dualboot_init.c
│   ├── debian_splash.c
│   └── build_initramfs.sh
├── kernel/                 # Kernel configuration and build environments
│   ├── config-4.14.190-running
│   └── build_kernel.sh
├── scripts/                # Utility scripts for flashing, log collection, and debugging
│   ├── connect_debian.sh
│   ├── flash_linux_direct.sh
│   └── collect_native_boot_logs.sh
└── vendor/                 # Firmware extraction tools and Adreno GPU firmware maps
```

---

## Quick Start Guide

### 1. Requirements
* Samsung Galaxy A70 (`SM-A705F`) with unlocked bootloader (`OEM Unlock` enabled in Developer Options).
* Host Linux machine (Ubuntu / Debian / Fedora) with `adb`, `fastboot`, `heimdall`, and `gcc-aarch64-linux-gnu` installed.

### 2. Building the Dual-Boot Initramfs
```bash
cd initramfs
# Compile custom dual-boot C init
aarch64-linux-gnu-gcc -static -O2 -Wall dualboot_init.c -o dualboot_init
# Package ramdisk
./build_initramfs.sh
```

### 3. Flashing to Phone (Download Mode via Heimdall)
1. Turn off the phone.
2. Hold **Volume Down + Volume Up** and plug in the USB cable connected to your PC.
3. On the turquoise warning screen, press **Volume Up** to enter Download Mode.
4. Run the flashing script:
```bash
chmod +x scripts/flash_linux_direct.sh
./scripts/flash_linux_direct.sh
```

### 4. Connecting via USB Console & Ethernet
Once booted, the phone exposes a composite USB gadget:

* **Instant Root Console (Serial):**
```bash
screen /dev/ttyACM0 115200
# or
minicom -D /dev/ttyACM0
```

* **Network / SSH Connection:**
```bash
# Configure host USB interface
sudo ip addr add 192.168.7.1/24 dev enp0s20f0u5
sudo ip link set enp0s20f0u5 up

# Connect via SSH
ssh debian@192.168.7.2
```

---

## License

This project is licensed under the Apache 2.0 License. Linux kernel sources and LineageOS device trees are licensed under GPLv2 and Apache 2.0 respectively.
