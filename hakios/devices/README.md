# Device repositories

`devices/manifest.conf` names every device. QEMU, SALAA and Nemo profiles are kept in this tree:
QEMU is the runnable virtual reference; SALAA and Nemo are experimental MT6785 bring-up profiles
requesting the current multi-core and read-only UFS paths. Neither phone's boot path or peripheral
support has been validated. Other device repositories may be cloned with
`tools/hakios-device.py sync`. A device of your own: `tools/hakios-device.py new`
([Port a device](../docs/porting/index.md)).
