# Samsung Galaxy A70 — Dual-Boot Architecture Specification

## 1. Architectural Strategy

The objective is to achieve a reliable, independent dual-boot system on the Samsung Galaxy A70 (`a70q`) using internal UFS storage without risking Android data integrity:

1. **Operating System A (Primary):** crDroid 13 (Android 13)
   - Stored in: `sda21` (`boot`), `sda23` (`system`), `sda24` (`vendor`), and `sda29` (`userdata`).
2. **Operating System B (Secondary):** Genuine Debian GNU/Linux 13 (Trixie) ARM64
   - **Phase 1 (Staged Verification):**
     - Kernel & Initramfs: Staged to `sda22` (`recovery`, 79 MiB).
     - Root Filesystem: 4.0 GiB sparse ext4 image file stored at `/data/debian-rootfs.img`.
     - **Risk to Android:** ZERO. Partition table is not repartitioned or altered.
   - **Phase 2 (True Dedicated Dual-Boot):**
     - Dedicated partition `sda30` (ext4, label `DEBIAN_ROOT`) created by shrinking `sda29` after Phase 1 is validated.

---

## 2. Boot Selection Mechanism

Samsung bootloaders provide built-in hardware recovery switching:

```
                      +-----------------------------+
                      |   Samsung S-Boot / XBL      |
                      +-----------------------------+
                                     |
                       [Hardware Key Check at Boot]
                                     |
               +---------------------+---------------------+
               |                                           |
         Normal Power On                        Power On + Vol Up
               |                                           |
               v                                           v
    +----------------------+                   +-----------------------+
    |   sda21 (boot)       |                   |  sda22 (recovery)     |
    |  crDroid 13 Android  |                   |  Native Debian 13     |
    |  Kernel + Ramdisk    |                   |  Kernel + Initramfs   |
    +----------------------+                   +-----------------------+
               |                                           |
               v                                           v
    +----------------------+                   +-----------------------+
    |  Mounts system,      |                   |  Mounts Loopback      |
    |  vendor, userdata    |                   |  /data/debian-rootfs  |
    +----------------------+                   +-----------------------+
               |                                           |
               v                                           v
    +----------------------+                   +-----------------------+
    | Android 13 Userspace |                   | Debian 13 Systemd     |
    +----------------------+                   +-----------------------+
```

### Safety Advantages of Recovery-Carrier Staging:
1. **Zero Bootloader Modification:** Does not require flashing unsigned third-party bootloader code or altering Samsung Knox PIT mappings.
2. **True Dual-Boot:** Normal power-on always boots Android. Holding `Volume Up` boots Debian Linux.
3. **Fail-Safe Isolation:** If Debian rootfs or kernel fails, Android remains completely intact.
4. **Capacity:** The recovery partition `sda22` is **79 MB (80,884 KB)**, easily accommodating `debian-boot.img` (17 MB).

---

## 3. Early Boot Network Console (RNDIS Gadget)

In every boot scenario, the Debian initramfs will:
1. Initialize Qualcomm USB ConfigFS DWC3 gadget (`a600000.dwc3`).
2. Bind RNDIS function with Google VID `0x18d1`, PID `0xd001`.
3. Assign static IP `192.168.42.2/24` to the phone interface (`usb0`/`rndis0`).
4. Start an early emergency telnet daemon on `192.168.42.2:23`.
5. This allows immediate debugging from the Fedora laptop (`192.168.42.1`) even if the display server or GUI crashes.
