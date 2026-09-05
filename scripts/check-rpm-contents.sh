#!/usr/bin/env bash
# Fails when the built RPM is missing a runtime file the app relies on.
# Usage: scripts/check-rpm-contents.sh <numi-kde-*.rpm>
set -euo pipefail

rpm_file="${1:?usage: check-rpm-contents.sh <rpm>}"
[[ -f "$rpm_file" ]] || { echo "no such file: $rpm_file" >&2; exit 1; }

required=(
  /usr/bin/numi-kde
  /usr/libexec/numi-kde-install-update
  /usr/libexec/numi-kde-engine
  /usr/share/applications/online.skorin.numi-kde.desktop
  /usr/share/metainfo/online.skorin.numi-kde.metainfo.xml
  /usr/share/knotifications6/numi-kde.notifyrc
  /usr/share/polkit-1/actions/online.skorin.numi-kde-update.policy
  /etc/pki/rpm-gpg/RPM-GPG-KEY-numi-kde
  /usr/share/kwin/scripts/numi-kde-window-memory/metadata.json
  /usr/share/kwin/scripts/numi-kde-window-memory/contents/code/main.js
  /usr/share/icons/hicolor/16x16/apps/online.skorin.numi-kde.png
  /usr/share/icons/hicolor/22x22/apps/online.skorin.numi-kde.png
  /usr/share/icons/hicolor/32x32/apps/online.skorin.numi-kde.png
  /usr/share/icons/hicolor/48x48/apps/online.skorin.numi-kde.png
  /usr/share/icons/hicolor/64x64/apps/online.skorin.numi-kde.png
  /usr/share/icons/hicolor/128x128/apps/online.skorin.numi-kde.png
  /usr/share/icons/hicolor/256x256/apps/online.skorin.numi-kde.png
)
forbidden=(
  /usr/share/icons/hicolor/scalable/apps/online.skorin.numi-kde.svg
  /usr/share/applications/numi-kde.desktop
)
required_deps=(kf6-kconfig kf6-kdbusaddons kf6-knotifications kf6-qqc2-desktop-style kf6-kdeclarative polkit dnf curl libqalculate)

files=$(rpm -qpl "$rpm_file")
deps=$(rpm -qpR "$rpm_file")
status=0

for f in "${required[@]}"; do
  if grep -qx -- "$f" <<<"$files"; then echo "  ok      $f"; else echo "  MISSING $f"; status=1; fi
done
for f in "${forbidden[@]}"; do
  if grep -qx -- "$f" <<<"$files"; then echo "  UNEXPECTED $f"; status=1; fi
done
for d in "${required_deps[@]}"; do
  if grep -qx -- "$d" <<<"$deps"; then echo "  ok      requires $d"; else echo "  MISSING requires $d"; status=1; fi
done

# The helper must be executable by root only via pkexec, but readable by all (0755).
mode=$(rpm -qp --qf '[%{FILENAMES} %{FILEMODES:perms}\n]' "$rpm_file" | awk '$1=="/usr/libexec/numi-kde-install-update"{print $2}')
if [[ "$mode" == "-rwxr-xr-x" ]]; then echo "  ok      helper mode $mode"; else echo "  BAD helper mode '$mode'"; status=1; fi

if [[ $status -eq 0 ]]; then echo "RPM contents OK: $(basename "$rpm_file")"; else echo "RPM contents check FAILED" >&2; fi
exit $status
