# MediaTek MT6785 shared layer

Shared SoC support for the Realme 7 SALAA (Helio G95) and Realme 6 Nemo (Helio G90T). The MT6768
Infinix Hot 20 port is a useful reference for HakiOS's MediaTek boot, UFS service, XNU integration,
and Bifrost bring-up. Its board register values and power sequences are not safe to copy directly.

## Default bring-up profile

`configs/mt6785.config` requests seven XNU CPUs, starts XNU on MPIDR core 7, enables the read-only
UEFI block service, and selects `xnusys` as root. One core remains with the resident loader. Storage
access is read-only; UFS writes and persistent writable storage stay off. It also overrides
`common_defconfig`'s unrelated Qualcomm, QEMU, and MT6768 drivers: those options would otherwise
compile or request unsupported hardware paths on MT6785. New features belong enabled only when a
matching MT6785 or board driver is present.

The profile requires firmware that can start AArch64 EFI applications, provide UFS Block I/O and
PSCI, plus a prepared `xnusys` GPT partition. Display output uses the firmware's GOP framebuffer
when present.

## MT6785 hardware facts

| Block | Reference values | Port status |
|---|---|---|
| CPU | 6 small cores (A55 @ 1.7 GHz) and 2 big cores (A76 @ 2.17 GHz); MPIDR Aff1 `0x000`–`0x700` | XNULOADER describes the map and requests 7 XNU CPUs; PSCI CPU start via SMC |
| GIC | GICv3 distributor `0x0c000000`, redistributor `0x0c040000`, config `0x0c53a650` | Included in the MT6785 XNULOADER platform table and SALAA memory map |
| UART | UART0 at `0x11002000`, 115200-8N1 from 26 MHz, MTK high-speed mode 3 | Fully implemented in `MT6785SerialPortLib`; XnuLoader streams direct hardware MMIO |
| Framebuffer | `0x75d30000`, size `0x02cd0000` (46.81 MiB), 1080×2400 @ 32 bpp, **Stride 1088** | Fully verified via `atag,videolfb` and `mtkfb`; SimpleFb GOP override passes rowBytes to XNU |
| UFS | UFSHCI `0x11270000` (region `0x2300`); M-PHY `0x11fa0000` (region `0xc000`) | UFS 2.1 Gear 3, Samsung KM8V8001JM-B813; register probe verified (DP, UTRLRDY, UTMRLRDY, UCRDY) |
| GPU | Mali-G76 MC4 / Bifrost at `0x13040000`; MFG clock/power block at `0x13fbf000` | Bifrost architecture; Panfrost / IOKit driver stubs |
| Display | MediaTek DSI/OVL; Tianma NT36672C (60.15 Hz) on SALAA | Generic GOP framebuffer handoff; native DSI retained by LK |
| Touch | Novatek NT36672C on SPI3 (`spi@1100f000`), reset GPIO 158, INT GPIO 8 | Profile defined; IOKit input mapping |
| Keys | Volume Up (GPIO 2), Volume Down, Power Key (PMIC MT6359) | Keypad and PMIC wrapper mappings |

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
