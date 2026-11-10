<!-- Made by tools/docs-gen.py from devices/salaa/device.conf. Do not edit: change that, then run tools/docs-gen.py. -->


# Realme 7 (RMX2151, SALAA) (`salaa`)

| | |
|---|---|
| Vendor | realme |
| SoC | `mt6785` |
| Configuration | `make salaa_defconfig` |
| Maintainer | HakiOS MT6785 port contributors |
| Repository | [`devices/salaa/`](https://github.com/oosexgang/hakios-xnu-kernel/tree/main/devices/salaa) |

## What works

| Part | State | |
|---|---|---|
| display | works | SALAA SimpleFb GOP verified (1080x2400, Stride 1088, Tianma NT36672C @ 0x75d30000, length 0x02cd0000) |
| cpu | works | MT6785 6+2 MPIDR affinity (6x A55 @ 1.7 GHz + 2x A76 @ 2.17 GHz); boot core 7, PSCI via SMC conduit |
| storage | works | MT6785 UFSHCI at 0x11270000 (UFS 2.1 Gear 3, Samsung KM8V8001JM-B813, direct SCSI LUNs 0/1); UFS Block I/O probe and Darwin root /dev/ufs_xnusys |
| touch | works | Tianma NT36672C 1080x2400 touch screen on MT6785 with coordinate normalization to 65535 and multi-touch reporting to /dev/hakios-touch |
| buttons | works | MT6785 Keypad (0x10010000), GPIO110 Volume Up, MT6359 PWRAP (0x1000D000) Power Key & Volume Down, and CPU frequency scaling via APMIXEDSYS |
| gpu | works | ARM Mali-G76 MC4 (Bifrost v7, 4 cores @ 900 MHz) at 0x13040000; SPM power domains (MFG_ASYNC, MFG, MFG_CORE0..3); HakiosMali DRM service |
| battery | works | MT6359 PMIC fuel gauge via PWRAP reading voltage, current, and temperature; HakiosBattery IOPMPowerSource reporting |
| thermal | works | MT6785 AUXADC/LVTS thermal sensors at 0x1100B000 exposed via sysctl kern.hakios.thermal_soc |
| usb | works | Generic UEFI USB class stack and XnuLoader fastboot USB communication over MT6785 SSUSB / xHCI |
| wifi | partly | MT6785 CONNSYS at 0x18002000 (MT6631/MT6635 combo radio); SPM power sequence, CONSYS_IP_VER and WMT handshake |
| bluetooth | partly | MT6785 CONNSYS BTIF / STP transport interface |
| audio | no | no MT6785 audio path |
| camera | no | no camera path |
| haptic | no | no haptic path |

The state is what has been seen on this device, written down in `devices/salaa/device.conf`; a part the table does not name has not been looked at.
