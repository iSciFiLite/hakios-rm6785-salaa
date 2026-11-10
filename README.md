# HakiOS RM6785 SALAA Unified — Inclusive UEFI Firmware and XNU Ecosystem

**Unified repository for running HakiOS on Realme 7 (RMX2151 / RM6785 / Helio G95 / MT6785).**

This monorepo combines two tightly coupled, interrelated projects into an inclusive development tree:
1. **SALAA UEFI Firmware (`uefi/`)** — Mu-Silicium EDK II platform overlay, 1088-stride SimpleFb GOP driver, MT6785 UART/UFS drivers, memory descriptors, and AArch64 BootShim.
2. **HakiOS MT6785 Operating System (`hakios/`)** — ARMv8.2-A XNU kernel port, XnuLoader EFI application, MT6785 SoC configuration, IOKit driver stubs, and XNUP payload packaging.

---

## Why Inclusive Unified Development?

Previously, development was conducted across two isolated repositories:
* [`iSciFiLite/rmx2151-salaa-uefi`](https://github.com/iSciFiLite/rmx2151-salaa-uefi)
* [`iSciFiLite/hakios-mt6785`](https://github.com/iSciFiLite/hakios-mt6785)

### The Problem with Disconnected Repositories
* **Layer Desynchronization**: The boot process is a continuous pipeline:
  $$\text{Stock LK} \longrightarrow \text{Android v2 Header} \longrightarrow \text{BootShim} \longrightarrow \text{SALAA UEFI FD} \longrightarrow \text{XnuLoader} \longrightarrow \text{XNU Kernel} \longrightarrow \text{Ramdisk}$$
  When major enhancements were made to the UEFI firmware (such as correcting the GOP framebuffer stride to 1088 pixels, configuring MT6785 UART0 at `0x11002000`, or updating UFSHCI probing), the `hakios-mt6785` repository was missing out. It had to wait for tag releases or pinned hash bumps.
* **Mismatched Boot Arguments & Protocols**: In `hakios-mt6785`, the kernel default config requested `CONFIG_UFS_ROOT=y` and `ufs=2`. However, because UEFI's UFS Block I/O is still a probe stub returning `EFI_NO_MEDIA`, booting without override would immediately panic. In isolation, coordinating these cross-layer requirements introduced friction and stale release dependencies.
* **Packaging Gaps**: Packaging the unified `SALAA_HAKIOS_XNU_BOOT.img` required downloading assets from multiple external releases rather than compiling and verifying from the active working tree.

### The Unified Solution
By uniting both components in this repository:
* **Atomic Cross-Layer Commits**: Changes to firmware protocols and OS driver consumers are developed, reviewed, and tested in the exact same commit.
* **Shared Evidence & Constants**: Memory map reservations (`0xF0000000` XNUP carveout), MMIO addresses (`0x11002000` UART, `0x11270000` UFS, `0x75d30000` Framebuffer), and PSCI configs remain strictly consistent.
* **Reversible Subtree Architecture**: Both `uefi/` and `hakios/` are maintained as clean subtrees. When inclusive development reaches complete cross-compatibility, changes can be cleanly synchronized back to their respective independent repositories at any time via `tools/split-subtrees.sh`.

---

## Current Completion Status & Gating Evidence

> **Assessment:** We are at the final stage of source closure and packaging. Hosted compilation of both the 4 MiB SALAA UEFI firmware and the AArch64v8.2-A XNU kernel is **100% complete and verified**. A full combined Android v2 boot smoke image (`SALAA_HAKIOS_XNU_BOOT.img`) has been packaged and verified against all checksums. The primary remaining gate is physical testing on an unlocked handset via `fastboot boot`.

| Subsystem / Layer | Implementation Status | Evidence & Verification |
|---|---|---|
| **Platform Skeleton** | ✅ Complete | AArch64 SEC/DXE, board package `SalaaPkg`, BDS, Shell. Builds cleanly against pinned Mu-Silicium. |
| **Physical Memory Map** | ✅ Complete | 8 GiB RAM span (`0x40000000–0x240000000`), 28 non-overlapping descriptors. `0xF0000000–0xF3FFFFFF` (64 MiB) carved out for XNUP handoff. |
| **GOP Display / Stride** | ✅ Complete | Physical framebuffer at `0x75d30000` (length `0x02cd0000`, 46,989,312 bytes). Stride fixed to **1088 pixels per scan line** (virtual size `1088,9600`) matching rooted handset `fb0` & `atag,videolfb` logs. |
| **UART Serial Port** | ✅ Complete (Code) | `MT6785SerialPortLib` at `0x11002000`, MTK high-speed mode 3, 115200-8N1 from 26 MHz source clock. |
| **UFS Storage** | ⚠️ Probe Stub | MMIO controller probe at `0x11270000`. Correctly decodes HCS bits (`DP=bit0`, `UCRDY=bit3`). Reports `EFI_NO_MEDIA` until PHY/link init and LUN discovery are implemented. |
| **PSCI / Multiprocessor** | ✅ Declared | 6+2 core affinity map (6× A55 @ 1.7 GHz + 2× A76 @ 2.17 GHz). Boot core 7; SMC conduit declared. |
| **XNU Kernel (MT6785)** | ✅ Complete | ARMv8.2-A+crypto+rcpc compilation clean via pinned Theos LLVM toolchain + iPhoneOS 16.5 SDK. Produces `mach_kernel` / `hakios-kernel`. |
| **XNUP Payload Container** | ✅ Complete | Packaged via `xnuloader/mkpayload.py` containing `hakios-kernel` + open `hello` ramdisk with cmdline `cpus=7 bootcpu=7 ufs=0 ufs_root=0`. |
| **Combined Fastboot Image** | ✅ Packaged | Android v2 boot image (`SALAA_HAKIOS_XNU_BOOT.img`, 7,464,960 bytes, SHA-256 `b4bf5a1081cd2155a21c0345a22921bbe815e561f9c0fd1e6632d9d5cff611d3`). Combines `gzip(BootShim + SALAA_EFI.fd + SALAA_XNUP.bin)` with verified F.11 ramdisk and DTB. |
| **Hardware Boot Test** | ❌ Gating Next Step | Requires one controlled `fastboot boot SALAA_HAKIOS_XNU_BOOT.img` from an external USB host on an unlocked bootloader. |

---

## Repository Structure

```text
hakios-rm6785-salaa/
├── Makefile                          # Unified build, check, and synchronization targets
├── README.md                         # Master documentation & progress ledger
├── .github/
│   └── workflows/
│       ├── unified-ci.yml            # Automated multi-layer audits and defconfig checks
│       └── unified-build-bootimg.yml # Hosted dual-compilation & boot.img pack pipeline
├── tools/
│   ├── split-subtrees.sh             # Sync inclusive commits back to rmx2151-salaa-uefi & hakios-mt6785
│   └── sync-subtrees.sh              # Pull upstream updates into subtrees
│
├── uefi/                             # SALAA UEFI Firmware Layer (Mu-Silicium Overlay)
│   ├── source/
│   │   ├── Platforms/Realme/SalaaPkg/   # Board DSC, DEC, FDF, MemoryMapLib, SEC stub
│   │   ├── Silicon/MediaTek/MT6785Pkg/  # Platform glue, UART SerialPortLib, UFS Block I/O stub
│   │   └── Silicon/Silicium/SiliciumPkg/# Stride-aware SimpleFbDxe (1088 stride override)
│   ├── Resources/Configs/Salaa.toml     # 4 MiB FD base and size configurations
│   ├── docs/salaa-device-evidence.md    # Sanitized handset logs, pstore, and hardware facts
│   └── tools/
│       ├── audit-memory-map.py          # Verifies 28 descriptors & XNUP carveout
│       ├── firmware-audit.py            # Static audit of firmware volume declarations
│       └── pack-salaa-bootimg.py        # Android v2 boot image packaging utility
│
└── hakios/                           # HakiOS OS & Kernel Layer (MT6785 Port)
    ├── xnu/                          # XNU kernel patch stack and MT6785 SoC overlays
    ├── xnuloader/                    # UEFI boot application with MT6785 platform mapping
    ├── drivers/                      # Shared XNU driver stubs (UFS, FB, UART)
    ├── devices/                      # Device profiles (devices/salaa, devices/nemo)
    ├── mtk/mt6785/                   # SoC definitions and mt6785.config
    ├── userland/                     # Open userland components (hello ramdisk)
    └── tools/                        # hakios-device.py, hakios-config.py, kernel-version.py
```

---

## Important Credible Resources

* **Device Tree**: [elohim-etz/android_device_realme_RM6785](https://github.com/elohim-etz/android_device_realme_RM6785)
* **Vendor Tree**: [elohim-etz/android_vendor_realme_RM6785](https://github.com/elohim-etz/android_vendor_realme_RM6785)
* **Kernel Tree**: [elohim-etz/android_kernel_realme_mt6785](https://github.com/elohim-etz/android_kernel_realme_mt6785)
* **HackiOS Core**: [oosexgang/hackios-xnu-kernel](https://github.com/oosexgang/hackios-xnu-kernel)
* **Project Mu Silicium**: [Project-Silicium/Mu-Silicium](https://github.com/Project-Silicium/Mu-Silicium/tree/main)
* **HakiOS UEFI**: [oosexgang/hakios-uefi](https://github.com/oosexgang/hakios-uefi)
* **HakiOS MT6768 Donor Reference**: [oosexgang/hakios-mt6768](https://github.com/oosexgang/hakios-mt6768)

> [!CAUTION]
> **Workspace Note**: Do **NOT** touch or modify any mainline Linux kernel folders (such as `linux-7.3` or `rui2-nemo-kernel` based on Nemo 6.3 mainline) in the Ubuntu environment. Our UEFI firmware and HakiOS bring-up are engineered directly from verified stock Realme F.11 firmware device evidence.

---

## Quick Start & Verification

### 1. Run Complete Subsystem Audits
From the root of this workspace:
```bash
make audit
```
This validates:
* All 28 non-overlapping memory descriptors in `uefi/source/Platforms/Realme/SalaaPkg/Library/MemoryMapLib/`.
* The `0xF0000000–0xF3FFFFFF` 64 MiB XNUP reservation window.
* Framebuffer physical allocation (`0x75d30000–0x789fffff`, 46.98 MiB) and 1088-pixel stride.
* HakiOS device profile consistency (`salaa` and `nemo`).
* HakiOS defconfig generation and driver metadata.

### 2. Configure HakiOS for SALAA
```bash
make defconfig DEVICE=salaa
```

### 3. Build the EFI Loader
```bash
make loader DEVICE=salaa
```

---

## Physical Hardware Testing Procedure

> [!WARNING]
> **DO NOT FLASH PARTITIONS.** Never run `fastboot flash boot`, `fastboot flash lk`, or wipe partitions. All testing is performed via temporary RAM boot: `fastboot boot`.

### Step 1: Read-Only Fastboot Preflight
From an external computer running Android `platform-tools`, connect the phone in fastboot/bootloader mode:
```bash
fastboot devices
fastboot getvar product 2>&1
fastboot getvar unlocked 2>&1
fastboot getvar secure 2>&1
```
*Verify that `unlocked: yes` before proceeding.*

### Step 2: One-Shot RAM Boot Test
Download `SALAA_HAKIOS_XNU_BOOT.img` and its checksum from the release:
```bash
sha256sum -c SHA256SUMS
fastboot boot SALAA_HAKIOS_XNU_BOOT.img 2>&1 | tee salaa-fastboot-boot.log
```

### Step 3: Observe and Report
* **Display Output**: Does the screen show the Tianma NT36672C display surviving handoff, the HakiOS boot banner, XnuLoader messages, or does it remain black?
* **UART Serial**: If serial pins GPIO53 (RX) / GPIO54 (TX) are connected to a 1.8V TTL serial adapter at 115200 baud, capture all early boot output from `MT6785SerialPortLib` (`0x11002000`).
* **Fallback / Recovery**: If the device reboots back into Android, collect post-boot telemetry in Termux:
  ```bash
  su -c 'dmesg | grep -Ei "ufs|framebuffer|mtkfb|panic|watchdog" | tail -100'
  ```

---

## Inclusive Development & Subtree Management

### Splitting Changes Back to Standalone Repositories
When both inclusive developments (the cross-compatible UEFI loader and the HakiOS MT6785 port) reach release milestones, push all commits back to the upstream repositories:
```bash
make split-subtrees
```
Or directly:
```bash
bash tools/split-subtrees.sh
```

### Synchronizing Upstream Changes
To pull changes made directly into either standalone repository:
```bash
make sync-subtrees
```
