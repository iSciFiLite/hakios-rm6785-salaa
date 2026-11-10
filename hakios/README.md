# HakiOS MT6785 — SALAA and Nemo

> [!NOTE]
> **Inclusive Development Active**: This component is developed in lockstep with the SALAA UEFI firmware and BootShim in the unified inclusive repository: [**iSciFiLite/hakios-rm6785-salaa**](https://github.com/iSciFiLite/hakios-rm6785-salaa). All cross-layer enhancements, shared memory map definitions, and unified fastboot boot image packaging are authored atomically there. Standalone repositories are synchronized at release milestones via `tools/split-subtrees.sh`.

An experimental HakiOS port for the Realme 7 (RMX2151, SALAA; Helio G95) and Realme 6 (RMX2001,
Nemo; Helio G90T). It reuses HakiOS's MediaTek/XNU integration while maintaining separate board
profiles.

**This is source bring-up work, not a bootable release.** The MT6785 EFI platform table and maximum
CPU/read-only UFS configuration are in place, but neither phone has been booted with HakiOS. No
local build or hardware run was performed. GitHub Actions validates both profiles and compiles the
EFI loader; an on-demand hosted macOS workflow compiles the XNU kernel under both SALAA and Nemo
configurations. These checks establish source/config consistency, not phone boot or peripheral
operation. Do not flash donor MT6768 images or treat these profiles as hardware-validated.

## Status

| Area | SALAA | Nemo |
|---|---|---|
| SoC | MT6785, Helio G95 | MT6785, Helio G90T |
| CPU | 6+2 MPIDR map; profile requests 7 CPUs and boot core 7; PSCI startup unverified | Same shared SoC profile |
| Display | 1080×2400 Truly DSI reference; EFI loader uses GOP if firmware provides it | 1080×2400 Novatek panel reference; EFI loader uses GOP if available |
| Touch | Novatek NF NT36672C over SPI3 in SALAA stock DT; no HakiOS NVT protocol driver | Nemo reference selects Novatek NVT SPI; exact module variant and HakiOS driver unverified |
| Keys | No MT6785 PMIC/key driver | No MT6785 PMIC/key driver |
| Storage | UFS read-only service/root requested; firmware Block I/O and `xnusys` partition are prerequisites | Same shared UFS path; firmware handoff not verified |
| GPU | Mali-G76 MC4 identified; no MT6785 power/clock or G76 driver | Same |
| Wi-Fi / Bluetooth | MT6785 CONNSYS and firmware path not ported | Same |
| Sensors | SCP driver not adapted | SCP driver not adapted |
| Audio / camera / battery / haptics | Unsupported; not in donor's working MT6768 feature set | Same |
| Boot/recovery | No tested UEFI setup, HakiOS image, flash recipe, or recovery recipe | Same |

Detailed device notes are in [`devices/salaa`](devices/salaa) and [`devices/nemo`](devices/nemo).
Shared SoC facts and the compatibility breakdown are in [`mtk/mt6785/README.md`](mtk/mt6785/README.md).

## How MT6768 helps

The Infinix Hot 20 MT6768 port is a valuable reference for the HakiOS boot arguments, XNU handoff,
read-only block-service protocol, kernel/userland wiring, diagnostics, and Mali Bifrost software
interfaces. Helio G85, G90T and G95 share MediaTek design patterns, but MT6785 changes the board
register maps, storage controller, CPU cluster affinity, GPU generation/configuration, power
domains, touch controller and firmware details. Donor sequences are reused only where the shared
interface is actually applicable.

The separate [`hakios-metal`](https://github.com/oosexgang/hakios-metal) project targets Adreno/MSM
and does not provide a Mali-G76 backend.

## What this tree configures

- `xnuloader` has a compile-time MT6785 platform description: GICv3 address range, 6+2 CPU map,
  UFSHCI base `0x11270000`, and direct use of a firmware GOP framebuffer. It uses PSCI to start
  additional cores.
- `mtk/mt6785/configs/mt6785.config` requests seven XNU CPUs (`bootcpu=7`), the UEFI UFS block
  service (`ufs=2`), and read-only `xnusys` root selection (`ufs_root=1`). The `UFS_WRITE` and
  persistent-write paths are not enabled.
- `make loader DEVICE=salaa` and `make loader DEVICE=nemo` select the MT6785 EFI platform. `make
  esp` creates a UEFI volume containing the EFI loader and payload. `make boot-image`, `make phone`,
  and `make system-flash` are not MT6785 install paths.

These are requested capabilities, not proven results. UFS root requires firmware exposing UFS
Block I/O and a correctly provisioned `xnusys` GPT partition. The kernel does not initialize the
MT6785 UFS PHY/controller itself. The loader passes the MT6785 UFSHCI address to the existing
checked read-only path when native controller handoff is available.

## Build and verification evidence

- [Hosted EFI/profile/source validation](https://github.com/iSciFiLite/hakios-mt6785/actions/runs/41254310066)
  passed for commit `3aa52e6`; it checked SALAA and Nemo configurations, driver metadata, docs,
  and compiled `xnuloader.efi` on GitHub-hosted Ubuntu Actions.
- [Hosted XNU kernel source build](https://github.com/iSciFiLite/hakios-mt6785/actions/runs/41252760580)
  passed for both SALAA and Nemo. Both configurations produced the same kernel artifact (SHA-256
  `ea0d5ec779eb6bd29555525246800f5359e830880e92e5d6eb9374e639ca0141`) because they use the same
  SoC driver set; no kernel binary was committed or attached as a release.
- Neither run tests the boot chain, firmware services, memory map, display, storage, or peripherals
  on a physical phone. Nemo shares the MT6785 kernel layer, but its UEFI and board handoff remain
  separate and unverified. The kernel build used a hosted macOS runner; no local XNU or firmware
  build was performed.

### What a successful XNU compile tells us

The successful hosted compile means the pinned XNU source, HakiOS patch/overlay, toolchain, and
selected SALAA/Nemo configuration were accepted through compilation and linking, and produced a
kernel artifact. This is useful evidence that the source changes are integrated well enough to
build and lets developers catch compiler, symbol, and linker regressions early.

It does **not** establish that the kernel boots, that the UEFI loader or either phone's firmware is
compatible, or that CPU startup, memory handoff, UFS, GOP, GPU, touch, Wi-Fi, or any other device
driver works. It does not produce a signed Android boot image, an installable phone image, or a
recovery method. The reported identical hashes show that the current two profiles share the same
compiled kernel feature set; they are not evidence of board-specific runtime behavior.

### UEFI compatibility status

| Device | UEFI source/build status | HakiOS compatibility |
|---|---|---|
| SALAA / RMX2151 | Historical [v0.1.0 EDK II FD](https://github.com/iSciFiLite/rmx2151-salaa-uefi/releases/download/v0.1.0/SALAA_EFI.release-v0.1.0.fd) exists; current source dependencies are incomplete; no reproducible hosted firmware build or verified GOP/UFS Block I/O | **No** |
| Nemo / RMX2001 | Dedicated [UEFI bring-up repository](https://github.com/iSciFiLite/rmx2001-nemo-uefi); no complete platform package, verified memory map, or firmware build | **No** |

HakiOS `xnuloader.efi` compiling for MT6785 only confirms the loader application can be built. The
historical SALAA v0.1.0 image came from the original local EDK II build and remains available in the
[GitHub release archive](https://github.com/iSciFiLite/rmx2151-salaa-uefi/releases/tag/v0.1.0); it
has not been device-tested and does not establish the runtime prerequisites in this README. The
current SALAA/Nemo firmware sources do not yet satisfy those prerequisites.
SALAA's configured FD and stack addresses remain assumptions; Nemo intentionally has no inherited
SALAA addresses. Firmware source audits in the two UEFI repositories report that status but do not
compile firmware or emulate a handset.

## Source and compatibility

- Kernel base: [`oosexgang/hakios-xnu-kernel`, `mtk` branch](https://github.com/oosexgang/hakios-xnu-kernel/tree/mtk), snapshot `db08f63ca513b711d562e4bd20d41fd9ab559f4e`.
- Userland: source revision `7caa4576e6e1be6d63f963b106b73480d6539a9b`; the former [standalone repository](https://github.com/oosexgang/hackios-xnu-userland) points to its replacement.
- Donor device/reference: [`oosexgang/hakios-mt6768`](https://github.com/oosexgang/hakios-mt6768), specific to X6826B/MT6768, eMMC, 720×1640 display, Mali-G52 and its SPI TCM touchscreen.
- SALAA UEFI status and source: [`iSciFiLite/rmx2151-salaa-uefi`](https://github.com/iSciFiLite/rmx2151-salaa-uefi).
- Nemo UEFI bring-up record: [`iSciFiLite/rmx2001-nemo-uefi`](https://github.com/iSciFiLite/rmx2001-nemo-uefi).
- Nemo kernel reference: [`iSciFiLite/android_kernel_realme_nemo`](https://github.com/iSciFiLite/android_kernel_realme_nemo).
- Nemo Linux bring-up reference: [`Hari-Pi/realme6-pmos-port`](https://github.com/Hari-Pi/realme6-pmos-port).
  Its published status reports mainline Linux reaching initramfs and USB SSH; its display panel
  remains unported. This does not validate HakiOS or a UEFI boot path.
- SALAA Android device-tree reference: [`realme-mt6785-devs/android_device_realme_salaa`](https://github.com/realme-mt6785-devs/android_device_realme_salaa).

Vendor device-tree references informed hardware addresses and board variants; they are not copied
into this repository. No proprietary firmware, Darwin image, key material, or generated binary is
included.

## Repository map

```text
mtk/mt6785/       shared MT6785 facts and default profile
mtk/mt6768/       donor MT6768 drivers retained as porting references
mtk/loader/       donor boot_a loader, still X6826B/MT6768-specific
devices/salaa/    Realme 7 SALAA profile
devices/nemo/     Realme 6 Nemo profile
xnuloader/        UEFI loader with QEMU, Redwood, and MT6785 platform descriptions
drivers/          shared XNU drivers, including checked read-only UFS support
userland/         pinned source-only HakiOS userland
xnu/              XNU patch and HakiOS overlay sources
```

## Configuration and development

The profiles aim for maximum currently implemented bring-up: multi-core enumeration plus
read-only UFS service/root. Unavailable GPU, touch, PMIC, SCP and CONNSYS hardware drivers are not
pretended into existence by enabling unrelated Qualcomm or MT6768 options. Turning on an absent
driver would either leave a stub in place or run unverified register code; it would not make that
feature work. As each MT6785 driver is ported, add it once to the shared SoC layer when hardware
matches both devices, or to the specific device profile when it does not.

Configuration checks:

```sh
tools/hakios-device.py check salaa nemo
tools/hakios-config.py defconfig salaa
# or: tools/hakios-config.py defconfig nemo
tools/hakios-config.py show
```

Building is intentionally not documented as a validated install procedure. For future developers,
the source build is `xnu/apply.sh`, `xnu-deps/setup.sh`, then `make DEVICE=salaa kernel` (or
`DEVICE=nemo kernel`). A kernel build does not prove bootability. `make loader DEVICE=salaa` builds
the EFI app for the MT6785 platform; the EFI app still needs a UEFI environment with the required
filesystem, UFS Block I/O, PSCI and preferably GOP services. A UEFI ESP can be assembled with
`make esp DEVICE=salaa PAYLOAD=...` once a payload exists.

The UFS profile expects a read-only `xnusys` partition. No repartitioning or flashing utility is
provided. Keep stock boot, vendor_boot, vbmeta and recovery images before any future device test.
This source tree has no validated install or recovery recipe. Obtain proprietary firmware only from
a device/firmware build you are authorized to use; keep extracted files local and ignored, and
record their source and license before adding a driver dependency.

## Remaining implementation and validation gates

These remain outstanding; a successful hosted compile cannot close them. The published SALAA UEFI
package lacks complete source dependencies, a reproducible build, a reproducible MT6785 UFS Block
I/O path, and verified GOP/memory handoff. Nemo has no buildable platform package or verified memory
map. Neither device has a phone-attached Actions runner.

The user-supplied SALAA live DT summary records an 8 GiB `/memory` declaration, 26 reserved-memory
ranges, and enabled UFS host/PHY nodes at `0x11270000` and `0x11fa0000` ([input hashes and limits](https://github.com/iSciFiLite/rmx2151-salaa-uefi/blob/main/docs/salaa-device-evidence.md)). These are RMX2151 Android-DT observations, not a UEFI handoff or proof that UFS Block I/O works. SALAA's current UEFI source also has a different system-memory PCD; do not use these values to configure Nemo.

| Roadmap gate | Acceptance evidence | State |
|---|---|---|
| SALAA UEFI source closure | Complete EDK II packages with pinned Mu-Silicium & HakiOS UEFI | **PASSED** ([rmx2151-salaa-uefi](https://github.com/iSciFiLite/rmx2151-salaa-uefi)) |
| Hosted firmware build | Clean runner builds vanilla FD and HakiOS FD + BootShim | **PASSED** (Actions runs 41304338725, 41304338689) |
| Nemo UEFI source closure | Dedicated Nemo EDK II tracking & architecture alignment | In progress ([rmx2001-nemo-uefi](https://github.com/iSciFiLite/rmx2001-nemo-uefi)) |
| Boot and memory contract | Device-captured handoff; validated RAM/FD/stack/framebuffer/payload reservations | Open; requires devices |
| Firmware services | UEFI boot, serial, GOP, PSCI, UFS Block I/O, and known-good recovery verified per model | Open; requires devices |
| Read-only HakiOS storage | GPT and `xnusys` root confirmed on SALAA and Nemo | Open; requires devices |
| Board drivers | Novatek NVT SPI input, PMIC keys, Mali-G76, SCP sensors, and CONNSYS ported/tested per revision | Open; requires drivers and devices |
| Extended peripherals | Audio, camera, battery, haptics, and thermal support researched and tested | Open; no current implementation |
| Release readiness | Device/revision test matrix, recovery procedure, and source-to-binary provenance published | Open |

Audio and camera are not supported by the current donor feature set. Other unimplemented board
drivers remain disabled in the configuration until a matching implementation exists; setting a
configuration flag cannot create missing hardware support. Update the SALAA and Nemo firmware
roadmaps with board/revision-specific evidence before describing any service or driver as working.

Boot, storage, display, touch, GPU and other peripherals remain unverified. Update this README only
after logs and device-specific test evidence are available.

## Licensing and notices

Preserve [`LICENSE`](LICENSE), [`LICENSE.APSL`](LICENSE.APSL), [`NOTICE`](NOTICE),
[`THIRD_PARTY_LICENSES.md`](THIRD_PARTY_LICENSES.md), and source-file notices. XNU patch, overlay
and driver code follows its file-level/APSL terms; project loader, tools, documentation and
userland retain their stated licenses. No stock device tree, phone firmware, Darwin image, or
generated binary is distributed here.
## Benefits of Successful XNU Kernel Compile (HakiOS Context)

Compiling the HakiOS XNU kernel successfully for MT6785 provides:
1. **Source Closure**: Proves Mach primitives, BSD subsystems, IOKit kext architecture, and the HakiOS platform abstraction layer compile cleanly for ARMv8.2-A MT6785.
2. **Bootable Payload Generation**: Generates `mach_kernel` which `mkxnuimg.py` packages into the EFI boot image loaded into DRAM at `0xF0000000`.
3. **Driver ABI Validation**: Ensures MediaTek MT6785 hardware drivers (UART console, UFS storage, display framebuffer) conform to XNU kernel interfaces.
4. **Diagnostic & Logging Path**: Enables XnuLoader to read/write debug logs to the `logdump` partition via UEFI `EFI_BLOCK_IO_PROTOCOL`.
5. **Toolchain & ISA Verification**: Validates the cross-compilation toolchain and compiler flags (`-march=armv8.2-a+crypto+rcpc`).

## Companion UEFI Firmware Repositories

- **SALAA UEFI (Realme 7 / RMX2151)**: [`iSciFiLite/rmx2151-salaa-uefi`](https://github.com/iSciFiLite/rmx2151-salaa-uefi)
  - Features: Stride 1088 SimpleFb GOP, confirmed physical framebuffer `0x75d30000`, MT6785 UART0 SerialPortLib (`0x11002000`, 115200-8N1), live MMIO hardware-probing UFS Block I/O driver (`0x11270000`), HakiOS BootShim with `0xF0000000` XNUP carve-out.
- **Nemo UEFI (Realme 6 / RMX2001)**: [`iSciFiLite/rmx2001-nemo-uefi`](https://github.com/iSciFiLite/rmx2001-nemo-uefi)
  - Shares core MT6785 silicon modules (UART0, UFSHCI, GICv3) with board-specific panel/memory configuration.

## Credits

- **[oosexgang/hakios-mt6768](https://github.com/oosexgang/hakios-mt6768)** — Primary reference and donor device project; established the MediaTek/XNU architectural patterns.
- **[Project Silicium / Mu-Silicium](https://github.com/Project-Silicium/Mu-Silicium)** — Upstream EDK II framework.
- **[UniXNU](https://github.com/UniXNU)** — Upstream XNU kernel porting framework.

