#!/bin/bash
# Privileged helper for numi-kde self-update — invoked exclusively via pkexec
# (polkit action online.skorin.numi-kde.update).
#
# Usage: numi-kde-install-update <version>        e.g. 0.1.81 or v0.1.81
#
# The caller only supplies a version number. This helper downloads the RPM and
# SHA256SUMS from the project's GitHub release into a root-owned directory,
# verifies the checksum and the package name, refuses downgrades, and installs.
# Nothing from the unprivileged caller is ever installed directly.
set -euo pipefail

REPO="DMYTROSKORIN/numi-kde"
PACKAGE="numi-kde"

die() { echo "numi-kde-install-update: $*" >&2; exit "${2:-1}"; }

version="${1:-}"
version="${version#v}"
if [[ ! "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  die "invalid version '${1:-}'" 2
fi

installed=$(rpm -q --qf '%{VERSION}' "$PACKAGE" 2>/dev/null || true)
if [[ -n "$installed" ]]; then
  newest=$(printf '%s\n%s\n' "$installed" "$version" | sort -V | tail -n1)
  if [[ "$installed" == "$version" || "$newest" != "$version" ]]; then
    die "refusing to install $version over installed $installed" 3
  fi
fi

arch=$(uname -m)
rpm_name="${PACKAGE}-${version}-${arch}.rpm"
base_url="https://github.com/${REPO}/releases/download/v${version}"

tmp=$(mktemp -d /var/tmp/numi-kde-update.XXXXXX)
chmod 700 "$tmp"
trap 'rm -rf "$tmp"' EXIT

curl -fsSL --proto '=https' --tlsv1.2 --max-time 300 \
  -o "$tmp/$rpm_name" "$base_url/$rpm_name" \
  || die "download of $rpm_name failed" 4
curl -fsSL --proto '=https' --tlsv1.2 --max-time 60 \
  -o "$tmp/SHA256SUMS" "$base_url/SHA256SUMS" \
  || die "download of SHA256SUMS failed" 4

expected=$(awk -v f="$rpm_name" '$2 == f { print $1 }' "$tmp/SHA256SUMS")
[[ -n "$expected" ]] || die "$rpm_name is not listed in SHA256SUMS" 5
actual=$(sha256sum "$tmp/$rpm_name" | awk '{ print $1 }')
[[ "$expected" == "$actual" ]] || die "checksum mismatch for $rpm_name" 5

pkg_name=$(rpm -qp --qf '%{NAME}' "$tmp/$rpm_name" 2>/dev/null || true)
[[ "$pkg_name" == "$PACKAGE" ]] || die "unexpected package name '$pkg_name'" 6

dnf install -y --nogpgcheck "$tmp/$rpm_name"
