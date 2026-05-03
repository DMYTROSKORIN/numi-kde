# Handover: numi-kde v0.1.15 → next session

Last updated: 2026-05-03.

## State of the project

Version **0.1.15** is released and on GitHub. All 61 tests pass. The install script works:

```sh
curl -fsSL https://github.com/DMYTROSKORIN/numi-kde/releases/latest/download/install.sh | bash
```

## Recently completed (v0.1.15)

- Removed light theme (dark-only UI, `lightTheme` property deleted everywhere)
- Fixed cross-currency arithmetic: `500 EUR - 100 USD` now returns EUR result
- Expanded `/help` with Area, Data (GB/MB), Speed examples
- Global hotkey: restored `setShortcut(NoAutoloading)` alongside `setGlobalShortcut`
- Window position (partial work — see below, still broken at startup)

---

## Open problem: window position never restored on startup

**Symptom**: Every time the app starts (and every time the settings window opens) it appears in the top-left corner (0, 0) of the screen, ignoring any saved position.

### Root cause analysis

**Layer 1 — QML never binds x/y at startup**

`Main.qml` lines 8–9:
```qml
width: windowSettings.savedWidth   // ← width IS restored
height: windowSettings.savedHeight // ← height IS restored
// x: ???                          // ← MISSING
// y: ???                          // ← MISSING
```

The `Settings` object (`windowSettings`) has `savedX` and `savedY` properties.
They are WRITTEN correctly via `onXChanged`/`onYChanged` (lines 107–108), but never
READ back into the window's `x` and `y` at startup. Width and height have bindings;
position does not. This is the single clearest bug.

**Layer 2 — `visible: true` bypasses C++ startup position logic**

`Main.qml` line 12: `visible: true`

This causes the window to appear immediately when the QML engine loads the root
component — before any C++ code in `main.cpp` can run. The `toggleWindow()` function
in `main.cpp` has the `prepareShow()` / `setPosition()` logic, but it is only called
for **hide → show** cycles (tray click / hotkey), **not for the initial startup**.

**Layer 3 — KWin rule approach used `positionrule=3` (Apply Initially)**

`documentmodel.cpp` (`setKWinKeepAboveRule`): the rule written to `kwinrulesrc` uses
`positionrule=3`, which KWin applies once at first window creation and then ignores.
The rule is overwritten before every `show()` with the latest saved position, which
works for toggle cycles — but never runs at initial startup (see Layer 2).

**Layer 4 — QML Settings ↔ C++ QSettings sync race**

QML `Settings` writes to `~/.config/numi-kde/numi-kde.conf` lazily (buffered).
C++ `QSettings("numi-kde","numi-kde")` reads the same file. If QML hasn't flushed
when C++ reads (e.g. in `prepareShow()`), C++ sees stale values. This means even
for toggle cycles the position may not be restored correctly when position was last
updated via QML (e.g. after dragging the window with `startSystemMove()`).

**Layer 5 — Settings window has zero position persistence**

`SettingsWindow.qml` is a bare `Window` with no `Settings` object, no `x:`/`y:`
bindings, no save/restore logic. It always opens at whatever position the compositor
assigns.

### Recommended fix (clean, minimal)

#### Fix 1 — X11: add x/y QML bindings (2 lines)

In `Main.qml`, after line 9, add:
```qml
x: windowSettings.savedX >= 0 ? windowSettings.savedX : undefined
y: windowSettings.savedY >= 0 ? windowSettings.savedY : undefined
```

On X11, `QWindow::setPosition()` works from the client side. These bindings restore
position at startup and keep it in sync. The existing `positionRestored` guard on
lines 107–108 correctly prevents the compositor's initial placement from overwriting
the saved value.

#### Fix 2 — Wayland: switch KWin rule to positionrule=4 (Remember)

In `documentmodel.cpp` `setKWinKeepAboveRule()`, change:
```cpp
// OLD — write explicit position each time
if (savedX >= 0 && savedY >= 0) {
    ruleLines << QStringLiteral("position=%1,%2").arg(savedX).arg(savedY);
    ruleLines << QStringLiteral("positionrule=3");
}
```
to:
```cpp
// NEW — let KWin remember position itself
ruleLines << QStringLiteral("positionrule=4");
```

`positionrule=4` = Remember. KWin itself tracks and restores the last user position
for windows matching this rule. No C++ position reading/writing needed.
Remove the `savedX`/`savedY` read from `setKWinKeepAboveRule()` and remove
`saveWindowPosition()` from the toggle flow (KWin handles it).

#### Fix 3 — Initial startup: show from C++ (not QML `visible: true`)

In `Main.qml`: remove `visible: true` (or change to `visible: false`).
In `main.cpp`, after locating `mainWindow`, add:
```cpp
// For Wayland: write KWin rule (now just keep-above + positionrule=4) before show
documentModel.prepareShow();  // already exists, just runs on Wayland
mainWindow->show();
// For X11: position binding in QML handles it; setPosition() is optional fallback
```

This removes the startup race: C++ always runs before first `show()`.

#### Fix 4 — Settings window position

In `SettingsWindow.qml`, add `QtCore` import and a `Settings` block:
```qml
import QtCore

Settings {
    id: settingsPos
    category: "SettingsWindow"
    property int savedX: -1
    property int savedY: -1
}
```
Add to the `Window`:
```qml
x: settingsPos.savedX >= 0 ? settingsPos.savedX : undefined
y: settingsPos.savedY >= 0 ? settingsPos.savedY : undefined
onXChanged: settingsPos.savedX = x
onYChanged: settingsPos.savedY = y
```

#### Fix 5 — Eliminate QML/C++ sync race

Remove `saveWindowPosition()` from `main.cpp` entirely (it's redundant if QML
bindings + KWin positionrule=4 handle everything). Remove the `savedX`/`savedY`
reads from C++ `prepareShow()` / `setKWinKeepAboveRule()`.

---

## Other known issues / next steps

- **HiDPI**: editor overlay alignment not verified for scaling factors > 1.0
- **Extended tests**: multi-line variable chains, complex date arithmetic edge cases
- **Mocked crypto tests**: existing crypto tests require live network
- **docs/architecture.md**: currently accurate, keep it updated

## Build instructions

```sh
# Debug build + tests
cmake -B build/kde -S kde -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build/kde -j$(nproc)
./build/kde/numi-kde-tests

# Release RPM
cmake -B build/kde-release -S kde -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build/kde-release -j$(nproc)
cd build/kde-release && cpack -G RPM
```

## Release checklist (after any fix)

1. Bump version in `kde/CMakeLists.txt` (`project(... VERSION X.Y.Z ...)`)
2. Add entry to `CHANGELOG.md`
3. `cmake --build build/kde-release -j$(nproc)`
4. `./build/kde-release/numi-kde --probe` → must print "probe ok"
5. `./build/kde-release/numi-kde-tests` → must show 0 failed
6. `cd build/kde-release && cpack -G RPM`
7. Generate SHA256SUMS: `sha256sum numi-kde-X.Y.Z-x86_64.rpm install.sh uninstall.sh`
8. `git commit -m "vX.Y.Z: ..."` (no Co-Authored-By lines)
9. `git push origin main`
10. `gh release create vX.Y.Z --title "vX.Y.Z" --notes "..." RPM install.sh uninstall.sh SHA256SUMS`

## Notes for the next model

- Never add `Co-Authored-By` lines to commits
- Always bump version and build RPM after functional changes
- The test suite binary is `numi-kde-tests`, not `numi_test`
- Build dirs: `build/kde` (debug), `build/kde-release` (release+RPM)
- KWin rules file: `~/.config/kwinrulesrc`
- App settings file: `~/.config/numi-kde/numi-kde.conf`
- User language: Russian
