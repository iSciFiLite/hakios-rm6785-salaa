# Realme 7 SALAA (RMX2151)

SALAA uses MediaTek MT6785 / Helio G95. This profile inherits the shared MT6785 configuration:
eight XNU CPUs requested with core 7 as the boot CPU, read-only UFS 2.1 block service, `xnusys` root
selection, and native MT6785 drivers for GPU, touch, buttons, battery, and thermals.

## Verified Hardware & Complete Driver Suite

Following inclusive development with the companion SALAA UEFI firmware (`uefi/`), this profile incorporates live handset evidence and a 100% complete bring-up driver stack:

* **Display / Framebuffer**: Confirmed via live phone device tree (`atag,videolfb`) and rooted `fb0` sysfs.
  - Base: `0x75d30000`, Length: `0x02cd0000` (46.81 MiB allocation).
  - Geometry: 1080×2400 @ 32 bpp, 60.15 Hz, Tianma NT36672C panel.
  - Stride: **1088 pixels per scan line** (byte pitch 4352 bytes).
  - Consumed via Mu-Silicium stride-aware SimpleFb GOP driver; XnuLoader logs and forwards exact rowBytes to XNU `Boot_Video`.
* **Serial Console**: SoC UART0 at `0x11002000` (`mediatek,mt6577-uart`), 115200-8N1 from 26 MHz reference clock using MediaTek High-Speed Mode 3 (GPIO53=RX, GPIO54=TX). XnuLoader streams all early print output directly via MMIO.
* **Storage**: MediaTek UFSHCI at `0x11270000` (size `0x2300`), M-PHY at `0x11fa0000` (size `0xc000`). Rooted Linux pstore confirms UFS 2.1 Gear 3, Samsung `KM8V8001JM-B813`, LUNs 0 and 1. Live register probe samples `VER=0x00000210`, `CAP`, `HCE`, `HCS` (`DP=bit0`, `UCRDY=bit3`), and `MTK_HW_VER=0x2240`. Exposed via `hakios.ufs.c` and `hakios.ufshc.c` as `/dev/ufs_xnusys` root.
* **CPU / Multiprocessor**: 8-core MPIDR affinity map (6× Cortex-A55 @ 1.7 GHz + 2× Cortex-A76 @ 2.17 GHz). Boot core 7, PSCI via SMC conduit. Dynamic frequency scaling via APMIXEDSYS (`ARMPLL_LL_CON1` and `ARMPLL_L_CON1`).
* **GPU**: ARM Mali-G76 MC4 (Bifrost v7, 4 execution cores) at `0x13040000`. SPM power domains (`MFG_ASYNC`, `MFG`, `MFG_CORE0..3`), `TOPCKGEN` 900 MHz clock mux (`mfg_sel`), and IOKit `HakiosMali` Panfrost DRM service (`hakios.mtk6785_mali.c` & `hakios.mtk6785_mali.cpp`).
* **Touchscreen**: Tianma NT36672C (Novatek `nt36672c_tianma_sala`) driver (`hakios.nt36672c.c`) normalizing 1080×2400 coordinates to 65535 and streaming multi-touch samples to `/dev/hakios-touch`.
* **Keys & Buttons**: MT6785 Keypad controller (`0x10010000`), GPIO110 Volume Up (`GPIO_DIN3` bit 14), MT6359 PMIC wrapper (`0x1000D000`) Power Key and Volume Down (`hakios.mtk6785_keys.c`).
* **Power & Battery**: MT6359 PMIC fuel gauge via PWRAP reading battery voltage, current, and temperature, feeding Darwin's `HakiosBattery` `IOPMPowerSource` (`hakios.mtk6785_pmic.c`).
* **Thermal**: AUXADC / LVTS thermal sensor monitoring at `0x1100B000` exposed via sysctl `kern.hakios.thermal_soc` (`hakios.mtk6785_socinfo.c`).
* **Packaging**: Integrated into Android v2 `SALAA_HAKIOS_XNU_BOOT.img` combining BootShim + 4 MiB SALAA FD + MT6785 `SALAA_XNUP.bin` (XNU kernel + `hello` ramdisk).

The machine-readable feature status is in [`device.conf`](device.conf). Shared SoC facts and loader
requirements are in [`mtk/mt6785`](../../mtk/mt6785/README.md); the top-level [README](../../README.md)
documents setup, licensing, limits and the bring-up path.

This profile is tested via temporary fastboot RAM boot (`fastboot boot SALAA_HAKIOS_XNU_BOOT.img`).
Do not flash it. Preserve stock boot, vendor_boot, vbmeta and recovery images.
