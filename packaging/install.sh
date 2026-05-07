#!/usr/bin/env bash
set -euo pipefail

REPO="DMYTROSKORIN/numi-kde"
PACKAGE_NAME="numi-kde"
DRY_RUN=0

usage() {
  cat <<EOF
Usage: install.sh [OPTIONS]

Install numi-kde from GitHub Releases (Fedora / RPM-based systems only).

Options:
  --dry-run    Show what would be done without making changes
  --help       Show this message

Environment:
  NUMI_KDE_VERSION  Install a specific version (e.g. v0.1.6).
                    Defaults to the latest release.

Supported distributions:
  Fedora (dnf)

For other distributions, build from source:
  https://github.com/DMYTROSKORIN/numi-kde#build-from-source-arch-ubuntu-debian-and-others
EOF
}

log()  { printf '  %s\n' "$*"; }
step() { printf '\n\033[1m==> %s\033[0m\n' "$*"; }
die()  { printf '\033[31merror:\033[0m %s\n' "$*" >&2; exit 1; }

for arg in "$@"; do
  case "$arg" in
    --dry-run) DRY_RUN=1 ;;
    --help)    usage; exit 0 ;;
    *) die "unknown option: $arg" ;;
  esac
done

# ── Distro detection ─────────────────────────────────────────────────────────
if [[ ! -f /etc/os-release ]]; then
  die "/etc/os-release not found — unsupported OS"
fi
# shellcheck source=/dev/null
source /etc/os-release

ARCH=$(uname -m)
if [[ "$ARCH" != "x86_64" ]]; then
  die "unsupported architecture: $ARCH (only x86_64 is supported)"
fi

case "${ID:-}" in
  fedora)
    PKG_MGR="dnf"
    ;;
  *)
    die "unsupported distribution: ${ID:-unknown}.
numi-kde ships RPM packages for Fedora only.
On other distributions, build from source:
  https://github.com/DMYTROSKORIN/numi-kde#build-from-source-arch-ubuntu-debian-and-others"
    ;;
esac

if ! command -v "$PKG_MGR" &>/dev/null; then
  die "package manager not found: $PKG_MGR"
fi

# Optional KDE/Plasma warning
if [[ -z "${XDG_CURRENT_DESKTOP:-}" ]] || ! echo "${XDG_CURRENT_DESKTOP:-}" | grep -qi "KDE\|Plasma"; then
  log "warning: KDE Plasma desktop not detected — the app may not work as intended"
fi

# ── Resolve release version ───────────────────────────────────────────────────
step "Resolving release"

if [[ -n "${NUMI_KDE_VERSION:-}" ]]; then
  VERSION="$NUMI_KDE_VERSION"
  log "using pinned version: $VERSION"
else
  if ! command -v curl &>/dev/null; then
    die "curl is required but not installed"
  fi
  VERSION=$(curl -fsSL "https://api.github.com/repos/${REPO}/releases/latest" \
    | grep '"tag_name"' | head -1 | sed 's/.*"tag_name": *"\([^"]*\)".*/\1/')
  [[ -n "$VERSION" ]] || die "could not resolve latest release version"
  log "latest version: $VERSION"
fi

PKG_FILE="${PACKAGE_NAME}-${VERSION#v}-x86_64.rpm"
DOWNLOAD_BASE="https://github.com/${REPO}/releases/download/${VERSION}"
PKG_URL="${DOWNLOAD_BASE}/${PKG_FILE}"
SUMS_URL="${DOWNLOAD_BASE}/SHA256SUMS"

# ── Download ──────────────────────────────────────────────────────────────────
step "Downloading"
log "package:   $PKG_FILE"
log "checksums: SHA256SUMS"

if [[ "$DRY_RUN" -eq 1 ]]; then
  log "[dry-run] would download $PKG_URL"
  log "[dry-run] would download $SUMS_URL"
  log "[dry-run] would verify checksum"
  log "[dry-run] would run: sudo dnf install ./$PKG_FILE"
  log "[dry-run] would run: numi-kde --probe"
  printf '\n\033[32mDry run complete — no changes made.\033[0m\n'
  exit 0
fi

TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

curl -fsSL --progress-bar "$PKG_URL"   -o "$TMPDIR/$PKG_FILE"
curl -fsSL                "$SUMS_URL"  -o "$TMPDIR/SHA256SUMS"

# ── Checksum verification ─────────────────────────────────────────────────────
step "Verifying checksum"
cd "$TMPDIR"
grep "$PKG_FILE" SHA256SUMS | sha256sum --check --status \
  || die "checksum verification failed for $PKG_FILE"
log "ok"
cd - >/dev/null

# ── Install ───────────────────────────────────────────────────────────────────
step "Installing"
log "package: $PKG_FILE"
sudo dnf install -y "$TMPDIR/$PKG_FILE"

# ── Smoke test ────────────────────────────────────────────────────────────────
step "Verifying installation"
numi-kde --probe || die "post-install probe failed"

INSTALLED=$(numi-kde --version)
printf '\n\033[32m%s installed.\033[0m\n' "$INSTALLED"
printf 'Launch from the application menu or run: numi-kde\n'
printf 'To start in background (tray only): numi-kde --hidden\n'
