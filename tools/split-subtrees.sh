#!/usr/bin/env bash
# ==============================================================================
# tools/split-subtrees.sh — Synchronize inclusive changes back to separate repos
# ==============================================================================
# When cross-compatible UEFI loader and HakiOS MT6785 port development reaches
# a release or milestone, use this script to push changes back to:
#   1. https://github.com/iSciFiLite/rmx2151-salaa-uefi (branch: development or main)
#   2. https://github.com/iSciFiLite/hakios-mt6785      (branch: main)
# ==============================================================================

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

UEFI_REMOTE="${UEFI_REMOTE:-https://github.com/iSciFiLite/rmx2151-salaa-uefi.git}"
HAKIOS_REMOTE="${HAKIOS_REMOTE:-https://github.com/iSciFiLite/hakios-mt6785.git}"
UEFI_BRANCH="${UEFI_BRANCH:-development}"
HAKIOS_BRANCH="${HAKIOS_BRANCH:-main}"

echo "==> Pushing uefi/ subtree to $UEFI_REMOTE ($UEFI_BRANCH)..."
git subtree push --prefix=uefi "$UEFI_REMOTE" "$UEFI_BRANCH"

echo "==> Pushing hakios/ subtree to $HAKIOS_REMOTE ($HAKIOS_BRANCH)..."
git subtree push --prefix=hakios "$HAKIOS_REMOTE" "$HAKIOS_BRANCH"

echo "==> Subtrees successfully split and synchronized to independent repositories!"
