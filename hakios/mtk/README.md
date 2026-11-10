# MediaTek layers

MediaTek support is organized by SoC under `mtk/<soc>/`, with handset-specific settings under
`devices/<codename>/`. The MT6768 donor and the MT6785 SALAA/Nemo profiles are separate; MTK does not
imply that their loaders or peripheral drivers can be shared unchanged.

| SoC layer | Devices | State |
|---|---|---|
| [`mt6768/`](mt6768/soc.conf) | Infinix Hot 20 X6826B | Donor implementation: eMMC, Mali-G52, X6826B loader and peripherals |
| [`mt6785/`](mt6785/README.md) | Realme 7 SALAA, Realme 6 Nemo | Hardware facts and conservative profiles; loader/peripheral port incomplete |

The MediaTek branch's current loader is `mtk/loader/`. It remains specific to the X6826B: its
storage routines use eMMC/MSDC, its framebuffer fallback is 720×1640, and its Mali setup targets
G52. The Makefile explicitly prevents other MediaTek profiles from routing through that path.

The MT6785 layer records the shared Helio G95/G90T SoC contract and the UFSHCI handoff address.
The generic kernel UFS code accepts a controller base supplied by a loader, and the XNU root hook
can select the `xnusys` partition with `ufs_root=1`. A loader still has to publish the checked
`ufs_shared_pa` page, preserve the live UFS controller state, and supply the GPT geometry before
that path can be enabled. See [`mt6785/README.md`](mt6785/README.md).

See the repository root README for the device status, development setup, licensing, and limitations.
