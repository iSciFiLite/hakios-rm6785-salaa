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
when present. Compile-time support does not establish that SALAA or Nemo firmware provides these
services.

## MT6785 hardware facts

| Block | Reference values | Port status |
|---|---|---|
| CPU | 6 small cores and 2 big cores; MPIDR Aff1 values `0x000` through `0x700` | XNULOADER describes the map and requests 7 XNU CPUs; PSCI CPU start is unverified |
| GIC | GICv3 distributor `0x0c000000`, redistributor `0x0c040000` | Included in the MT6785 XNULOADER platform table |
| UFS | UFSHCI `0x11270000`, region `0x2300`; M-PHY `0x11fa0000` | Read-only UEFI block-service path is configured; UFS Block I/O must be present |
| GPU | Mali-G76 MC4 / Bifrost at `0x13040000`; MFG clock/power block at `0x13fbf000` | No MT6785 GPU power sequence or G76 kernel port yet |
| Display | MediaTek DSI/OVL; panel varies by handset/revision | Generic GOP framebuffer handoff only; no MTK DSI driver |
| Touch | Novatek NVT SPI controller family | Hardware-specific HakiOS input path not ported |

SALAA's stock device tree identifies a 1080×2400 Truly DSI panel and Novatek NF NT36672C on SPI3;
panel/touch module firmware variants exist, so do not treat every RMX2151 board as identical. The
Nemo postmarketOS kernel configuration selects Novatek NVT SPI and a Novatek NT36672A panel. Exact
revision matching is still required.

## Loader and storage path

`xnuloader/loader.c` has an MT6785 platform table with the GIC map, CPU cluster layout, UFSHCI
address, and native GOP use. `make loader DEVICE=salaa` and `make loader DEVICE=nemo` select this
table; `make esp` packages the EFI application. The MT6768 `mtkloader` and its X6826B boot-image,
eMMC, framebuffer and flash commands remain device-specific.

The kernel can read a firmware-provided UFS Block I/O service through the existing checked shared
page. `ufs=2 ufs_root=1` requests the `xnusys` partition as read-only root. The native UFSHCI path
also receives the MT6785 controller base if the loader offers its transfer-list handoff. Neither
path initializes the UFS PHY or controller; firmware must leave UFS operational and expose Block
I/O. The loader must see a valid `xnusys` partition before the root profile can succeed.

The linked [SALAA UEFI source](https://github.com/iSciFiLite/rmx2151-salaa-uefi) is an experimental
EDK II platform package. Its current DSC does not include UFS DXE or GOP/display drivers, and its
runtime peripheral operation is not established. It is a hardware reference, not a ready-made HakiOS
boot environment. Nemo has no published UEFI profile in this repository.

## What can be reused from MT6768

- HakiOS's kernel boot arguments, EFI payload container, firmware Block I/O bridge, and read-only
  APFS root selection are shared software interfaces.
- CPU architecture, GICv3 and standard UFSHCI concepts are close enough to guide the port, with
  device-tree values and firmware state checked for MT6785.
- Mali G76 is Bifrost, like the donor's G52, so Panfrost concepts and parts of the HakiOS Mali
  client protocol are candidates for reuse. The G76 has a different core count, MFG domains,
  clocking, power sequencing and interrupt map; the MT6768 sequence is not enabled here.
- The donor's TCM touchscreen, MSDC/eMMC, PMIC keys, SCP and CONNSYS code is board/SoC-specific.
  SALAA and Nemo use UFS and Novatek NVT touch hardware. They need separate adaptation and
  firmware extraction before those features can be enabled safely.

## Reference sources

- [HakiOS MT6768 donor](https://github.com/oosexgang/hakios-mt6768) and
  [shared MTK kernel branch](https://github.com/oosexgang/hakios-xnu-kernel/tree/mtk)
- [SALAA UEFI source](https://github.com/iSciFiLite/rmx2151-salaa-uefi)
- [Nemo kernel and postmarketOS reference](https://github.com/iSciFiLite/android_kernel_realme_nemo)
- Local vendor device trees were used to record the MT6785 GIC/UFS/GPU addresses and SALAA panel
  and touch variants. They are not copied into this repository.

This layer is source bring-up support. A hosted macOS runner compiled the shared XNU kernel source
with the SALAA/MT6785 configuration; this did not produce a phone-ready system image or verify
either handset. All MT6785 boot and peripheral behavior remains unverified. Do not flash a donor
MT6768 image.
