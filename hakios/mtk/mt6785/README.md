# MediaTek MT6785 shared layer

Shared SoC support for the Realme 7 SALAA (Helio G95) and Realme 6 Nemo (Helio G90T). The MT6768
Infinix Hot 20 port served as a historical reference; this MT6785 layer incorporates complete, dedicated
drivers tailored to the MT6785 architecture, UFS 2.1 storage, Bifrost Mali-G76 MC4, and live handset evidence.

## Default bring-up profile

`configs/mt6785.config` enables all eight CPUs (6× Cortex-A55 @ 1.7 GHz + 2× Cortex-A76 @ 2.17 GHz) with core 7
as the boot CPU, UFS 2.1 UFSHCI block service with `xnusys` root selection, and the full suite of native MT6785
drivers:

* `CONFIG_HAKIOS_MTK6785_KEYS=y`: MT6785 keypad (`0x10010000`), GPIO110 Volume Up, MT6359 PWRAP (`0x1000D000`) Power Key, and CPU frequency scaling via APMIXEDSYS (`0x1000C000`).
* `CONFIG_HAKIOS_MTK6785_MALI=y`: ARM Mali-G76 MC4 (Bifrost v7, 4 execution cores) at `0x13040000`, 6 SPM power domains (`MFG_ASYNC`, `MFG`, `MFG_CORE0..3`), TOPCKGEN 900 MHz clock mux (`mfg_sel`), and IOKit `HakiosMali` Panfrost DRM service.
* `CONFIG_HAKIOS_NT36672C=y`: Novatek NT36672C multi-touch driver normalizing 1080×2400 coordinates to 65535 for `/dev/hakios-touch`.
* `CONFIG_HAKIOS_MTK6785_PMIC=y`: MT6359 PMIC fuel gauge via PWRAP reading battery voltage, current, and temperature, reporting to Darwin's `HakiosBattery` `IOPMPowerSource`.
* `CONFIG_HAKIOS_MTK6785_SOCINFO=y`: MT6785 SoC identification (`0x0785` / `0x2240`) and AUXADC thermal monitoring (`0x1100B000`) exposed via `kern.hakios.soc_*` and `kern.hakios.thermal_soc`.
* `CONFIG_HAKIOS_MTK6785_CONNSYS=y`: MT6785 CONNSYS wireless subsystem (`0x18002000`) for MT6631/MT6635 radio.
* `CONFIG_HAKIOS_UFS=y`: JEDEC UFSHCI 2.1 driver (`0x11270000`) interfacing with Samsung KM8V8001JM-B813 UFS storage for `/dev/ufs_xnusys` root.

## MT6785 hardware facts

| Block | Reference values | Port status |
|---|---|---|
| CPU | 6 small cores (A55 @ 1.7 GHz) and 2 big cores (A76 @ 2.17 GHz); MPIDR Aff1 `0x000`–`0x700` | 8 CPUs enabled; boot core 7; PSCI CPU start via SMC; dynamic frequency scaling via APMIXEDSYS |
| GIC | GICv3 distributor `0x0c000000`, redistributor `0x0c040000`, config `0x0c53a650` | Included in MT6785 XNULOADER platform table and SALAA memory map |
| UART | UART0 at `0x11002000`, 115200-8N1 from 26 MHz, MTK high-speed mode 3 | Fully implemented in `MT6785SerialPortLib`; XnuLoader streams direct hardware MMIO |
| Framebuffer | `0x75d30000`, size `0x02cd0000` (46.81 MiB), 1080×2400 @ 32 bpp, **Stride 1088** | Fully verified via `atag,videolfb` and `mtkfb`; SimpleFb GOP override passes rowBytes to XNU |
| UFS | UFSHCI `0x11270000` (region `0x2300`); M-PHY `0x11fa0000` (region `0xc000`) | UFS 2.1 Gear 3, Samsung KM8V8001JM-B813; direct SCSI LUNs 0/1; Darwin root `/dev/ufs_xnusys` |
| GPU | Mali-G76 MC4 / Bifrost at `0x13040000`; MFG clock/power block at `0x13000000` | 4 cores; 6 SPM power domains; 900 MHz MFGPLL; `HakiosMali` DRM service for Mesa / WindowServer |
| Touch | Tianma NT36672C (1080×2400) on SALAA | Dedicated touch driver `hakios.nt36672c.c` with coordinate normalization to `/dev/hakios-touch` |
| Keys | GPIO110 Volume Up, MT6359 PMIC Power Key and Volume Down | Debounced polling thread posting button events to `/dev/hakios-touch` |
| Battery | MT6359 PMIC via PWRAP `0x1000D000` | Real-time voltage, current, and temperature reporting to `HakiosBattery` `IOPMPowerSource` |
| Thermal | AUXADC/LVTS thermal sensors at `0x1100B000` | Sysctl `kern.hakios.thermal_soc` reporting CPU cluster temperature |

## Loader and storage path

`xnuloader/loader.c` has an MT6785 platform table with the GIC map, CPU cluster layout, UFSHCI
address (`0x11270000`), UART0 console (`0x11002000`), and native GOP use with 1088 stride.
`make loader DEVICE=salaa` and `make loader DEVICE=nemo` compile the EFI loader.

The companion [SALAA UEFI firmware](../../uefi/) provides:
- 4 MiB AArch64 firmware volume (`SALAA_EFI.fd`).
- 28-descriptor non-overlapping physical memory map with 64 MiB carveout at `0xF0000000`.
- Stride-aware SimpleFb GOP driver configured for 1088 pixels per line at `0x75d30000`.
- Hardware MMIO UFSHCI probe and MT6785 UART0 SerialPortLib.

The combined Android v2 boot container (`SALAA_HAKIOS_XNU_BOOT.img`) packages BootShim + SALAA UEFI FD + MT6785 XNUP (`mach_kernel` + `hello` ramdisk) for temporary `fastboot boot` execution without flashing device partitions.
