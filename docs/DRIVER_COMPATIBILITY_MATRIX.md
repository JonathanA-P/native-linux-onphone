# Samsung Galaxy A70 (SM-A705F) — Driver Compatibility Matrix

## Hardware & Driver Validation Matrix

| Subsystem | Hardware Component | Kernel Driver / Config | Status | Evidence / Verification | Outstanding Dependencies / Blocker |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **CPU / SoC** | Qualcomm SM6150 (2+6 cores) | `arch/arm64` core | **PASS** | Kernel compiles cleanly with Clang 14.0.6; genuine Debian 13 arm64 binary execution verified. | None. Fully native out-of-the-box. |
| **Storage (UFS)**| 128 GB UFS 2.1 | `CONFIG_SCSI_UFS_QCOM=y` | **PASS** | Monolithic in-kernel driver verified (`build.config`); partitions discovered via standard SCSI block layer. | None. |
| **USB Gadget** | Qualcomm DWC3 (`a600000.dwc3`) | `CONFIG_USB_CONFIGFS_RNDIS=y` | **PASS** | ConfigFS RNDIS gadget script verified; network interface `rndis0`/`usb0` configured at `192.168.42.2`. | None. |
| **Rootfs / Init**| Genuine Debian 13 Trixie | `systemd`, `ext4` | **PASS** | `debian-rootfs.img` formatted with `LABEL=DEBIAN_ROOT`; `/sbin/init` symlink resolves to `/usr/lib/systemd/systemd`. | None. |
| **Display Panel**| Aftermarket Non-OLED Replacement | `CONFIG_DRM_MSM=y`, `sde_kms` | **NOT TESTED** | Panel technology and controller IC are unknown. Works in Android 13; native Linux MIPI-DSI power-on sequence unverified. | Requires live boot test to confirm DRM KMS panel probe. |
| **Backlight** | Qualcomm PM6150L WLED | `qcom-wled` (`/sys/class/backlight/backlight`) | **PARTIAL** | Verified active under Android 13 (4095 levels). Initramfs writes 2048 to brightness node. Unverified in pure Linux. | Depends on panel power sequencing. |
| **Touchscreen** | Imagis IST40XX (`ist40xx-ts`) | `CONFIG_TOUCHSCREEN_IST40XX=y` | **NOT TESTED** | Verified active on `/dev/input/event2` (I2C 0x50) in Android 13. Event delivery under pure `libinput` unverified on native boot. | Requires live boot test to verify I2C probe and interrupt delivery. |
| **Power / AFC** | PM6150 / PM6150L PMIC | `sec_battery_sysfs_qc` | **PARTIAL** | Battery status exposed via sysfs; fast charging handled in-tree. Power management suspend loop unverified. | Wake-lock management in systemd. |
| **Wi-Fi** | Qualcomm WCN3990 | `qcacld-3.0` | **NOT TESTED** | Hardware present; unverified in pure Debian. | Requires firmware files from `/vendor/firmware/wlan/` and `wpa_supplicant`. |
| **Bluetooth** | Qualcomm WCN3990 | `hci_uart` / `btqca` | **NOT TESTED** | Unverified in pure Debian. | Requires BT firmware loading utility. |
| **Audio** | WCD937x / QDSP6 | ALSA / `techpack/audio` | **NOT TESTED** | Unverified in pure Debian. | Requires Qualcomm DSP firmware and ALSA UCM profiles. |
| **Cellular Modem**| Snapdragon X12 LTE | MSM IPC / QRTR | **NOT TESTED** | Unverified in pure Debian. | Requires non-HLOS modem firmware, QRTR daemon, and ModemManager. |
| **GPU 3D** | Qualcomm Adreno 612 | `kgsl` / `msm_drm` | **PARTIAL** | Software rasterizer (llvmpipe) works out of the box in Debian; hardware 3D requires Freedreno or libhybris. | Mesa Freedreno or libhybris EGL wrapper. |
| **Camera** | Triple Rear (32MP) + Front (32MP) | `cam_core` / Camera HAL3 | **NOT TESTED** | Android Camera HAL3 required. | Deferred to post-boot adaptation phase. |
| **Fingerprint** | Optical Under-Display (FOD) | Goodix / Samsung FOD | **NOT TESTED** | Requires custom high-brightness panel illumination (HBM). | Deferred to post-boot adaptation phase. |

---

## Native Boot Feasibility Assessment

- **Early Boot & Emergency Shell (Headless):** **READY (PASS)**. The kernel, UFS storage, ConfigFS USB RNDIS console, and rootfs mount are completely self-contained with zero external module dependencies.
- **Graphical Interactivity (Display & Touch):** **PENDING LIVE VERIFICATION (NOT TESTED)**. The replacement display panel controller is unknown; native framebuffer output can only be verified once the device boots.
