# Samsung Galaxy A70 (SM-A705F) — Build & Test Guide

This guide documents the automated, reproducible build and validation workflow for the native Debian 13 port.

All automation scripts are maintained in [scripts/](file:///home/jonathan/Documents/Projects/Apps/a70/native-linux/scripts/).

---

## 1. Quick Automated Audit & Verification

To verify the entire project state and all generated artifacts:
```bash
# 1. Audit kernel configuration
/home/jonathan/Documents/Projects/Apps/a70/native-linux/scripts/check_kernel_config.sh

# 2. Inspect device tree blobs (Base SM6150 DTB + DTBO overlays)
/home/jonathan/Documents/Projects/Apps/a70/native-linux/scripts/validate_device_tree.sh

# 3. Verify cryptographic checksums of all deliverables
/home/jonathan/Documents/Projects/Apps/a70/native-linux/scripts/verify_checksums.sh

# 4. Validate artifact file sizes and formats
/home/jonathan/Documents/Projects/Apps/a70/native-linux/scripts/validate_artifacts.sh
```

---

## 2. Connected Device Diagnostic Scripts

With the Samsung Galaxy A70 connected via USB with ADB authorized:
```bash
# Deep inspection of connected phone properties, crypto, mounts, display, and touch
/home/jonathan/Documents/Projects/Apps/a70/native-linux/scripts/inspect_device.sh

# Capture live kernel dmesg, recovery logs, and pstore ramoops
/home/jonathan/Documents/Projects/Apps/a70/native-linux/scripts/collect_device_logs.sh
```

---

## 3. Full Reproducible Build Pipelines

To rebuild components from source:

### Rebuild Kernel (`Image.gz-dtb` & `dtbo.img`):
```bash
/home/jonathan/Documents/Projects/Apps/a70/native-linux/scripts/build_kernel.sh
```

### Rebuild Secure Initramfs (`initramfs.cpio.gz`):
```bash
/home/jonathan/Documents/Projects/Apps/a70/native-linux/scripts/build_initramfs.sh
```

### Rebuild Android Header v1 Boot Image (`debian-boot.img`):
```bash
/home/jonathan/Documents/Projects/Apps/a70/native-linux/scripts/package_boot_image.sh
```

### Rebuild 4.0 GiB Sparse Ext4 Rootfs Image (`debian-rootfs.img`):
```bash
/home/jonathan/Documents/Projects/Apps/a70/native-linux/scripts/build_rootfs.sh
```
