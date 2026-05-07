#!/usr/bin/env bash
set -euo pipefail

PACKAGE_NAME="numi-kde"
PURGE=0

usage() {
  cat <<EOF
Usage: uninstall.sh [OPTIONS]

Remove numi-kde using the system package manager (Fedora / RPM-based only).
User settings and history are preserved by default.

Options:
  --purge    Also remove user configuration (~/.config/numi-kde and
             ~/.local/share/numi-kde) after explicit confirmation
  --help     Show this message
EOF
}

log()  { printf '  %s\n' "$*"; }
step() { printf '\n\033[1m==> %s\033[0m\n' "$*"; }
die()  { printf '\033[31merror:\033[0m %s\n' "$*" >&2; exit 1; }

for arg in "$@"; do
  case "$arg" in
    --purge) PURGE=1 ;;
    --help)  usage; exit 0 ;;
    *) die "unknown option: $arg" ;;
  esac
done

# ── Distro detection ─────────────────────────────────────────────────────────
if [[ ! -f /etc/os-release ]]; then
  die "/etc/os-release not found — unsupported OS"
fi
# shellcheck source=/dev/null
source /etc/os-release

case "${ID:-}" in
  fedora)
    PKG_MGR="dnf"
    ;;
  *)
    die "unsupported distribution: ${ID:-unknown}.
numi-kde ships RPM packages for Fedora only.
On other distributions, remove the binary you built manually."
    ;;
esac

# ── Check whether installed ───────────────────────────────────────────────────
step "Checking installation"
if ! rpm -q "$PACKAGE_NAME" &>/dev/null; then
  die "$PACKAGE_NAME is not installed"
fi
INSTALLED_VERSION=$(rpm -q --qf '%{VERSION}' "$PACKAGE_NAME")
log "found: $PACKAGE_NAME $INSTALLED_VERSION"

# ── Remove ────────────────────────────────────────────────────────────────────
step "Removing"
sudo dnf remove -y "$PACKAGE_NAME"
log "package removed"

# ── Purge user data (optional) ────────────────────────────────────────────────
if [[ "$PURGE" -eq 1 ]]; then
  step "Purging user data"
  printf '\033[33mThis will permanently delete your numi-kde settings and history.\033[0m\n'
  read -r -p 'Type YES to confirm: ' confirm
  if [[ "$confirm" == "YES" ]]; then
    rm -rf \
      "${XDG_CONFIG_HOME:-$HOME/.config}/numi-kde" \
      "${XDG_DATA_HOME:-$HOME/.local/share}/numi-kde"
    log "user data removed"
  else
    log "purge cancelled — user data preserved"
  fi
fi

printf '\n\033[32mnumi-kde uninstalled.\033[0m\n'
if [[ "$PURGE" -eq 0 ]]; then
  printf 'User settings and history have been preserved.\n'
fi
