# Native Linux on Phone (Samsung Galaxy A70 — `a70q` / `SM-A705F`)

Genuine bare-metal **Debian GNU/Linux 13 (Trixie)** port for the **Samsung Galaxy A70** (`SM-A705F` / `a70q`), featuring full `systemd` PID 1, custom dual-boot initramfs, **Phosh** Wayland mobile environment, Qualcomm Adreno 612 hardware acceleration via **Turnip Vulkan + Zink**, and headless USB gadget connectivity (CDC-NCM Ethernet + CDC-ACM serial root console).

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

### 1. Dual-Boot & Partition Strategy
* **Non-Destructive Coexistence:** The stock Android userdata partition (`/dev/block/sda29`) remains untouched.
* **Dedicated Linux Partition:** Debian resides on `/dev/block/sda30` formatted as native `ext4`.
* **Stage 1 Bootloader:** A custom statically-linked C initramfs (`initramfs/dualboot_init.c`) runs first from boot/recovery. It probes block devices, presents a hardware splash / recovery diagnostic, and mounts Debian rootfs before calling `switch_root`.

### 2. Hardware Acceleration Pipeline
* **Turnip over KGSL:** The downstream Android kernel does not provide DRM MSM render nodes (`renderD128`). We leverage Mesa Turnip's native KGSL backend (`TU_DEBUG=kgsl`) communicating directly with `/dev/kgsl-3d0`.
* **Zink Desktop OpenGL:** Desktop OpenGL / GLES applications route through Mesa's Zink Gallium driver (`MESA_LOADER_DRIVER_OVERRIDE=zink`), translating OpenGL calls to Turnip Vulkan.

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
