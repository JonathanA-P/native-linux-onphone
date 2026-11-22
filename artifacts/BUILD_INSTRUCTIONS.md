# Samsung Galaxy A70 (SM-A705F / a70q) — Native Boot Package Build Instructions

## 1. Directory Structure

All artifacts are maintained inside `/home/jonathan/Documents/Projects/Apps/a70/native-linux/artifacts/`:
- `kernel/Image.gz-dtb` — Monolithic Linux 4.14.190 ARM64 kernel with appended Qualcomm SM6150 DTB.
- `kernel/dtbo.img` — Samsung SM6150 DTBO table containing 12 hardware revision overlays (`r00` through `r14`).
- `kernel/build.config` — Complete extracted `.config` showing monolithic in-tree configuration (0 `=m` modules).
- `boot/initramfs.cpio.gz` — Early boot ramdisk with ARM64 static Busybox, PM6150L WLED backlight, USB ConfigFS RNDIS (`192.168.42.2`), loopback mounter, and systemd switcher.
- `boot/debian-boot.img` — Packaged Android boot header v1 image.
- `rootfs/debian-13-trixie-arm64.tar.xz` — Genuine Debian 13 (Trixie) ARM64 root filesystem archive.
- `rootfs/debian-rootfs.img` — 4.0 GiB sparse ext4 filesystem labeled `DEBIAN_ROOT`.
- `SHA256SUMS` — Cryptographic verification hashes.

---

## 2. Kernel Compilation Command (Reproducible)

Run inside container `quay.io/droidian/build-essential:current-amd64`:
```bash
cd /buildd/kernel/sm6150-kernel
make clean mrproper
make a70q_droidian_defconfig
make -j$(nproc) Image.gz-dtb
mkdtboimg create /buildd/artifacts/kernel/dtbo.img arch/arm64/boot/dts/samsung/*.dtbo
```

---

## 3. Initramfs Packaging Command

```bash
/home/jonathan/Documents/Projects/Apps/a70/native-linux/initramfs/build_initramfs.sh
```

---

## 4. Boot Image Packaging Command

```bash
/home/jonathan/Documents/Projects/Apps/a70/native-linux/initramfs/build_bootimg.sh
```

---

## 5. Debian 13 (Trixie) ARM64 Rootfs Bootstrap Command

```bash
mmdebstrap \
    --architectures=arm64 \
    --include=systemd,systemd-sysv,udev,kmod,iproute2,net-tools,isc-dhcp-client,openssh-server,sudo,curl,wget,nano,locales,tzdata,ca-certificates,dbus,libinput-bin,polkitd,procps,htop \
    trixie \
    /tmp/rootfs \
    http://deb.debian.org/debian
```
