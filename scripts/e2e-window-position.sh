#!/usr/bin/env bash
# End-to-end check of Wayland window position memory on a live Plasma session.
#
# Needs the installed numi-kde (or a running dev build) and KWin Wayland.
# Drives the app through kglobalacceld (the real hotkey action) and reads
# window geometry back through a KWin script, so nothing is faked:
#   show → move to (X,Y) → hide → show → compare → restart app → show → compare.
#
# Usage: scripts/e2e-window-position.sh [X Y]      (default 160 140)
set -euo pipefail

X="${1:-160}"; Y="${2:-140}"
SERVICE="online.skorin.numi-kde"
COMPONENT="/component/numi_kde"
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
ok=0; bad=0
pass() { printf '  PASS  %s\n' "$1"; ok=$((ok+1)); }
fail() { printf '  FAIL  %s (%s)\n' "$1" "$2"; bad=$((bad+1)); }

kwin_script() { # file → prints NUMI-E2E lines it logged
  local id
  id=$(busctl --user call org.kde.KWin /Scripting org.kde.kwin.Scripting loadScript s "$1" | awk '{print $2}')
  busctl --user call "org.kde.KWin" "/Scripting/Script$id" org.kde.kwin.Script run
  sleep 0.7
  busctl --user call "org.kde.KWin" "/Scripting/Script$id" org.kde.kwin.Script stop >/dev/null 2>&1 || true
  journalctl --user --since '-3s' --no-pager -o cat 2>/dev/null | grep 'NUMI-E2E' | tail -1 | sed 's/^NUMI-E2E //'
}
cat > "$work/probe.js" <<'EOF'
let out = "none";
for (const w of workspace.windowList()) {
  if (String(w.resourceClass) === "online.skorin.numi-kde" && w.caption === "Numi-KDE")
    out = Math.round(w.frameGeometry.x) + "," + Math.round(w.frameGeometry.y) + " above=" + w.keepAbove;
}
console.warn("NUMI-E2E " + out);
EOF
cat > "$work/move.js" <<EOF
for (const w of workspace.windowList()) {
  if (String(w.resourceClass) === "online.skorin.numi-kde" && w.caption === "Numi-KDE") {
    w.frameGeometry = { x: $X, y: $Y, width: w.frameGeometry.width, height: w.frameGeometry.height };
    console.warn("NUMI-E2E moved " + Math.round(w.frameGeometry.x) + "," + Math.round(w.frameGeometry.y));
  }
}
EOF
toggle() { busctl --user call org.kde.kglobalaccel "$COMPONENT" org.kde.kglobalaccel.Component invokeShortcut s toggle-window; sleep 1.5; }
geometry() { kwin_script "$work/probe.js"; }
visible() { [[ "$(geometry)" != "none" ]]; }

echo "=== numi-kde window position e2e ($X,$Y) ==="
busctl --user list | grep -q "$SERVICE" || { echo "numi-kde is not running (service $SERVICE missing)"; exit 1; }
loaded=$(busctl --user call org.kde.KWin /Scripting org.kde.kwin.Scripting isScriptLoaded s numi-kde-window-memory | awk '{print $2}')
[[ "$loaded" == "true" ]] && pass "KWin script numi-kde-window-memory is loaded" || fail "KWin script loaded" "$loaded"

visible || toggle
g=$(geometry); [[ "$g" == *"above=true" ]] && pass "window shown with keep-above" || fail "keep-above" "$g"

kwin_script "$work/move.js" >/dev/null
sleep 0.5
toggle   # hide → script reports geometry to the app
sleep 1
toggle   # show → script restores it
g=$(geometry)
[[ "$g" == "$X,$Y above=true" ]] && pass "position restored after hide/show" || fail "position after hide/show" "$g"

toggle   # hide
pkill -TERM -x numi-kde || true
sleep 1
setsid nohup numi-kde --hidden >/dev/null 2>&1 &
sleep 3
toggle   # show
g=$(geometry)
[[ "$g" == "$X,$Y above=true" ]] && pass "position restored after app restart" || fail "position after restart" "$g"
toggle   # leave it hidden

saved=$(grep -A1 '^\[WindowMemory\]' "${XDG_CONFIG_HOME:-$HOME/.config}/numi-kde/numi-kde.conf" 2>/dev/null | tail -1)
[[ "$saved" == *"$X,$Y"* ]] && pass "position persisted in numi-kde.conf" || fail "persisted position" "$saved"

printf '\n=== Results: %d passed, %d failed ===\n' "$ok" "$bad"
[[ $bad -eq 0 ]]
