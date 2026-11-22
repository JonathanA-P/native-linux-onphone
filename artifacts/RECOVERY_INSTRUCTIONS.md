# Samsung Galaxy A70 (SM-A705F / a70q) — Emergency Recovery Instructions

## 1. Safety Directives
- **Zero Partition Modification:** The current validation phase strictly preserves internal UFS partitions.
- **Stock Boot Integrity:** The Android primary boot partition (`sda21`) remains untouched.
- **Recovery Isolation:** When testing, Debian boot image is only staged to `recovery` (`sda22`) or run via test loopback.

---

## 2. Emergency Recovery Key Combinations

### Force Reboot
If the screen freezes or kernel halts during early boot:
- Hold `Power` + `Volume Down` for **7 seconds**.
- The phone forces an electrical PMIC reset.
- Release keys; the device boots normally into crDroid Android 13.

### Download Mode (Odin / Heimdall Mode)
If boot partition or recovery partition needs restoration:
1. Power off device completely.
2. Hold `Volume Up` + `Volume Down` simultaneously.
3. Plug in USB-C cable connected to Fedora host.
4. When teal screen appears, press `Volume Up` once.
5. Restore stock recovery or boot image:
   ```bash
   heimdall flash --RECOVERY /home/jonathan/Backups/A70-Linux-Migration/stock_recovery.img
   ```

### Recovery Mode
1. Power off device.
2. Connect USB cable to PC or charger.
3. Hold `Power` + `Volume Up` until Samsung logo appears, then release `Power`.
4. Boots into TWRP/OrangeFox recovery for full ADB root access.
