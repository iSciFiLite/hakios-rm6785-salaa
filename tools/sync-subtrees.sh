#!/usr/bin/env bash
# ==============================================================================
# tools/sync-subtrees.sh — Pull upstream changes into uefi/ and hakios/ subtrees
# ==============================================================================

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

UEFI_REMOTE="${UEFI_REMOTE:-https://github.com/iSciFiLite/rmx2151-salaa-uefi.git}"
HAKIOS_REMOTE="${HAKIOS_REMOTE:-https://github.com/iSciFiLite/hakios-mt6785.git}"
UEFI_BRANCH="${UEFI_BRANCH:-development}"
HAKIOS_BRANCH="${HAKIOS_BRANCH:-main}"

echo "==> Pulling latest changes into uefi/ from $UEFI_REMOTE ($UEFI_BRANCH)..."
git subtree pull --prefix=uefi "$UEFI_REMOTE" "$UEFI_BRANCH" -m "Sync uefi subtree with upstream $UEFI_BRANCH"

echo "==> Pulling latest changes into hakios/ from $HAKIOS_REMOTE ($HAKIOS_BRANCH)..."
git subtree pull --prefix=hakios "$HAKIOS_REMOTE" "$HAKIOS_BRANCH" -m "Sync hakios subtree with upstream $HAKIOS_BRANCH"

echo "==> Both subtrees successfully updated!"
