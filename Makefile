# ==============================================================================
# HakiOS RM6785 SALAA Unified — Makefile
# Inclusive Development of UEFI Firmware & HakiOS XNU Kernel Ecosystem
# ==============================================================================

SHELL := /usr/bin/env bash
DEVICE ?= salaa
JOBS ?= $(shell nproc 2>/dev/null || echo 4)

.PHONY: all help audit check test-config uefi-audit hakios-check \
        split-subtrees sync-subtrees clean

all: help

help:
	@echo "HakiOS RM6785 SALAA Unified Development"
	@echo "========================================"
	@echo "Inclusive build & audit targets:"
	@echo "  make audit           - Run all UEFI memory-map, firmware, and HakiOS audits"
	@echo "  make check           - Validate HakiOS device profiles and defconfigs"
	@echo "  make test-config     - Test HakiOS SALAA and Nemo defconfig generation"
	@echo "  make uefi-audit      - Run SALAA UEFI firmware and memory map checks"
	@echo "  make hakios-check    - Check HakiOS MT6785 driver integrity and platform stubs"
	@echo "  make split-subtrees  - Synchronize subtrees back to rmx2151-salaa-uefi and hakios-mt6785"
	@echo "  make sync-subtrees   - Pull upstream changes into uefi/ and hakios/ subtrees"
	@echo ""
	@echo "HakiOS OS & Kernel targets (inside hakios/):"
	@echo "  make defconfig DEVICE=salaa  - Generate HakiOS configuration for SALAA"
	@echo "  make loader DEVICE=salaa     - Compile xnuloader.efi for MT6785"
	@echo "  make kernel DEVICE=salaa     - Compile HakiOS XNU kernel for MT6785"
	@echo ""
	@echo "UEFI Firmware targets (inside uefi/):"
	@echo "  See uefi/README.md for hosted or local Mu-Silicium build steps"

audit: uefi-audit hakios-check

uefi-audit:
	@echo "==> Auditing SALAA UEFI memory map..."
	python3 uefi/tools/audit-memory-map.py
	@echo "==> Auditing SALAA UEFI firmware declarations..."
	python3 uefi/tools/firmware-audit.py

hakios-check:
	@echo "==> Checking HakiOS device definitions..."
	python3 hakios/tools/hakios-device.py check salaa nemo
	@echo "==> Checking HakiOS driver metadata..."
	python3 hakios/tools/drivers-check.py
	@echo "==> Testing HakiOS SALAA configuration..."
	python3 hakios/tools/hakios-config.py defconfig salaa
	python3 hakios/tools/hakios-config.py show

check: audit

test-config:
	@echo "==> Testing SALAA defconfig..."
	python3 hakios/tools/hakios-config.py defconfig salaa
	@echo "==> Testing Nemo defconfig..."
	python3 hakios/tools/hakios-config.py defconfig nemo

defconfig:
	@echo "==> Configuring HakiOS for $(DEVICE)..."
	python3 hakios/tools/hakios-config.py defconfig $(DEVICE)

loader:
	@echo "==> Building HakiOS xnuloader for $(DEVICE)..."
	$(MAKE) -C hakios DEVICE=$(DEVICE) loader

kernel:
	@echo "==> Building HakiOS XNU kernel for $(DEVICE)..."
	$(MAKE) -C hakios DEVICE=$(DEVICE) JOBS=$(JOBS) kernel

split-subtrees:
	bash tools/split-subtrees.sh

sync-subtrees:
	bash tools/sync-subtrees.sh

clean:
	rm -f hakios/.config hakios/.config.old
