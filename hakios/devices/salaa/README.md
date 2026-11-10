# Realme 7 SALAA (RMX2151)

SALAA uses MediaTek MT6785 / Helio G95. This profile inherits the shared MT6785 configuration:
seven XNU CPUs requested with core 7 as the boot CPU, read-only UFS service, and `xnusys` root
selection.

The stock device tree identifies a 1080×2400 Truly DSI panel and Novatek NF NT36672C touchscreen
on SPI3. Touch firmware and panel/touch modules vary by board revision. HakiOS has no NVT SPI input
driver yet. The EFI loader accepts a UEFI GOP framebuffer, but the published SALAA UEFI profile does
not currently provide a validated GOP or UFS Block I/O path.

The machine-readable feature status is in [`device.conf`](device.conf). Shared SoC facts and loader
requirements are in [`mtk/mt6785`](../../mtk/mt6785/README.md); the top-level [README](../../README.md)
documents setup, licensing, limits and the bring-up path.

This profile is not a tested image or install procedure. Do not flash it. Preserve stock boot,
vendor_boot, vbmeta and recovery images before future experiments.
