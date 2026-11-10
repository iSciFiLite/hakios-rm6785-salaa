# Realme 6 Nemo (RMX2001)

Nemo uses MediaTek MT6785 / Helio G90T. This profile inherits the shared MT6785 configuration:
seven XNU CPUs requested with core 7 as the boot CPU, read-only UFS service, and `xnusys` root
selection.

The local Nemo postmarketOS reference selects a Novatek NT36672A DSI panel and Novatek NVT SPI
touch driver. Its touch module and firmware revision still need confirmation on the target device.
HakiOS has no NVT SPI input driver yet. The EFI loader can pass a UEFI GOP framebuffer, but no
Nemo UEFI platform integration is published in this repository.

The machine-readable feature status is in [`device.conf`](device.conf). Shared SoC facts and loader
requirements are in [`mtk/mt6785`](../../mtk/mt6785/README.md); the top-level [README](../../README.md)
documents setup, licensing, limits and the bring-up path.

This profile is not a tested image or install procedure. Do not flash it. Preserve stock boot,
vendor_boot, vbmeta and recovery images before future experiments.
