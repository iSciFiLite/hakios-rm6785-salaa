# Realme 7 SALAA (RMX2151)

SALAA uses MediaTek MT6785 / Helio G95. This profile inherits the shared MT6785 configuration:
seven XNU CPUs requested with core 7 as the boot CPU, read-only UFS service, and `xnusys` root
selection.

## Verified Hardware & Profile Status

Following inclusive development with the companion SALAA UEFI firmware (`uefi/`), this profile incorporates live handset evidence:

* **Display / Framebuffer**: Confirmed via live phone device tree (`atag,videolfb`) and rooted `fb0` sysfs.
  - Base: `0x75d30000`, Length: `0x02cd0000` (46.81 MiB allocation).
  - Geometry: 1080×2400 @ 32 bpp, 60.15 Hz, Tianma NT36672C panel.
  - Stride: **1088 pixels per scan line** (byte pitch 4352 bytes).
  - Consumed via Mu-Silicium stride-aware SimpleFb GOP driver; XnuLoader logs and forwards exact rowBytes to XNU `Boot_Video`.
* **Serial Console**: SoC UART0 at `0x11002000` (`mediatek,mt6577-uart`), 115200-8N1 from 26 MHz reference clock using MediaTek High-Speed Mode 3 (GPIO53=RX, GPIO54=TX). XnuLoader streams all early print output directly via MMIO.
* **Storage**: MediaTek UFSHCI at `0x11270000` (size `0x2300`), M-PHY at `0x11fa0000` (size `0xc000`). Rooted Linux pstore confirms UFS 2.1 Gear 3, Samsung `KM8V8001JM-B813`, LUNs 0 and 1. Live register probe samples `VER`, `CAP`, `HCE`, `HCS` (`DP=bit0`, `UCRDY=bit3`), and `MTK_HW_VER=0x2240`.
* **CPU / Multiprocessor**: 6+2 core MPIDR affinity map (6× A55 @ 1.7 GHz + 2× A76 @ 2.17 GHz). Boot core 7, PSCI via SMC conduit.
* **Touchscreen**: Novatek NT36672C on SPI3 (`spi@1100f000`), reset GPIO 158, INT GPIO 8.
* **Keys**: MT6359 PMIC power key, Volume Up (GPIO 2), Volume Down.
* **Packaging**: Integrated into Android v2 `SALAA_HAKIOS_XNU_BOOT.img` combining BootShim + 4 MiB SALAA FD + MT6785 `SALAA_XNUP.bin` (XNU kernel + `hello` ramdisk).

The machine-readable feature status is in [`device.conf`](device.conf). Shared SoC facts and loader
requirements are in [`mtk/mt6785`](../../mtk/mt6785/README.md); the top-level [README](../../README.md)
documents setup, licensing, limits and the bring-up path.

This profile is tested via temporary fastboot RAM boot (`fastboot boot SALAA_HAKIOS_XNU_BOOT.img`).
Do not flash it. Preserve stock boot, vendor_boot, vbmeta and recovery images.
