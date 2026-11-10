#!/bin/bash
# Build the XNU loader as a freestanding AArch64 UEFI application.
set -e
cd "$(dirname "$0")"
PLATFORM_CFLAGS=()
case "${HAKIOS_PLATFORM:-auto}" in
	auto|qemu-virt|redwood) ;;
	mt6785) PLATFORM_CFLAGS=(-DHAKIOS_PLATFORM_MT6785=1) ;;
	*) echo "xnuloader: unknown HAKIOS_PLATFORM=${HAKIOS_PLATFORM}" >&2; exit 2 ;;
esac
# the iPhone's identity for XNU, from the device tree of the builder's own IPSW (not in the repository)
[ -f identity.h ] || ./mkidentity.sh

clang --target=aarch64-unknown-windows -ffreestanding -fno-builtin -fno-stack-protector -fshort-wchar \
  -mgeneral-regs-only -O1 -Wall -Wextra -Wno-unused-parameter "${PLATFORM_CFLAGS[@]}" -c loader.c -o loader.obj
# /base: UEFI loads an image at its preferred address when that is free. The PE default (0x140000000)
# is in the middle of the phone's largest block of RAM, which XNU needs in one piece.
lld-link /subsystem:efi_application /entry:efi_main /nodefaultlib /base:0xA1000000 /out:loader.efi loader.obj
ls -la loader.efi
