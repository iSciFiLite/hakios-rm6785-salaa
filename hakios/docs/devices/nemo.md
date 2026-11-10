<!-- Made by tools/docs-gen.py from devices/nemo/device.conf. Do not edit: change that, then run tools/docs-gen.py. -->


# Realme 6 (RMX2001, Nemo) (`nemo`)

| | |
|---|---|
| Vendor | realme |
| SoC | `mt6785` |
| Configuration | `make nemo_defconfig` |
| Maintainer | HakiOS MT6785 port contributors |
| Repository | [`devices/nemo/`](https://github.com/oosexgang/hakios-xnu-kernel/tree/main/devices/nemo) |

## What works

| Part | State | |
|---|---|---|
| display | partly | Nemo references describe 1080x2400 Novatek panel; XNULOADER consumes UEFI GOP but device firmware GOP is unavailable/unverified |
| touch | partly | Nemo postmarketOS config selects Novatek NVT SPI; HakiOS has no NVT protocol driver yet |
| buttons | no | MTK PMIC/key input path not ported for this board |
| cpu | partly | MT6785 6+2 MPIDR map is in the XNULOADER profile; 7 CPUs requested, PSCI startup unverified |
| gpu | no | Mali-G76 MC4 is identified; MT6768 G52 power/clock sequence is not reused |
| storage | partly | MT6785 UFSHCI base and read-only UEFI block path are configured; Nemo firmware handoff is unverified |
| audio | no | no MT6785 audio path |
| battery | no | no MT6785 PMIC/battery path |
| wifi | no | MT6785 CONNSYS, firmware and network stack are not ported |
| bluetooth | no | MT6785 CONNSYS, firmware and transport are not ported |
| camera | no | no camera path |
| haptic | no | no haptic path |
| thermal | no | no MT6785 thermal path |

The state is what has been seen on this device, written down in `devices/nemo/device.conf`; a part the table does not name has not been looked at.
