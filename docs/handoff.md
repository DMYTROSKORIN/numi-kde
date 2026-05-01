# numi-kde Handoff

Last updated: 2026-05-01.

## Project Goal

Build a KDE/Linux-native Numi-like calculator: a plain-text document editor with live results, persistent scratchpad behavior, and Plasma integration.

The app is C++/Qt/QML backed by `libqalculate`. The repository is scoped to the native KDE project only.

## Current Native App State

Works now:

- Frameless compact dark window with KDE-style title controls.
- Editable source document on the left, synced result column on the right.
- C++ syntax highlighting and C++ evaluation through `libqalculate`.
- Variables, units, percentages, dates/time keywords, explicit date differences, `/help`, decimal precision setting.
- Live CoinGecko crypto rates for top crypto symbols, with manual crypto conversion fallback in `QalcBridge`.
- History drawer with persisted sessions and clear action.
- Settings window:
  - always on top;
  - launch at login;
  - show result separator;
  - font size;
  - result column width;
  - decimal places;
  - global hotkey recorder.
- Tray icon and menu.
- App/tray icons are transparent PNG resources.
- Global shortcut defaults to `Ctrl+Alt+1` and uses KDE `KGlobalAccel` when available.
- `Ctrl+N` clears the current document.
- `Tab` completes units, functions, variables, and date keywords.
- Result click copies the result; Total footer click copies the numeric total.
- Total includes numeric values from converted unit/currency/crypto result rows.
- Result column expands to fit long result text and the separator cannot be dragged over existing result content.
- `/help` shows a readable instruction panel with examples.
- Invalid explicit math such as `500/0` shows compact `Error`; incomplete input such as `20% from` stays blank.

## Wayland Always-on-Top

On KDE Wayland, clients cannot directly set "keep above" for themselves. The app now handles this by writing a managed KWin rule to `~/.config/kwinrulesrc` when "Always on top" is enabled:

- match: `wmclass` substring `numi-kde`, title `Numi`;
- action: `keepabove=true`, `keepaboverule=2`;
- then DBus call: `org.kde.KWin /KWin org.kde.KWin.reconfigure`.

On X11, `DocumentModel::setKeepAbove()` also uses `KX11Extras::setState(..., NET::KeepAbove)`.

## Important Files

```text
kde/CMakeLists.txt
kde/src/main.cpp                 QApplication, QML engine, tray, global shortcut context
kde/src/documentmodel.{h,cpp}    QML model, history, clipboard, settings helpers, KWin rule
kde/src/qalcbridge.{h,cpp}       libqalculate bridge, preprocessing, crypto conversion, date spans
kde/src/shortcutmanager.{h,cpp}  KGlobalAccel hotkey registration and conflict handling
kde/src/syntaxhighlighter.*      C++ syntax highlighting
kde/qml/Main.qml                 main window, flags, drawer, persisted geometry
kde/qml/DocumentPage.qml         editor/result layout, Ctrl+N, help panel, adaptive result width, Total footer
kde/qml/EditorPane.qml           TextArea overlay and Tab completion
kde/qml/ResultsPane.qml          result rendering/copy interaction
kde/qml/HistoryPane.qml          history list and clear button
kde/qml/SettingsWindow.qml       settings UI
kde/resources/*.png              app/tray icons
kde/tests/qalc_test.cpp          native test suite
```

## Verification

Run from repo root:

```sh
cmake --build build/kde --target numi-kde numi-kde-tests
ctest --test-dir build/kde --output-on-failure
./build/kde/numi-kde
```

Latest local result: build passes, `ctest` passes 2/2.

## Current Git State

The worktree is intentionally not clean. It contains the current implementation changes and new files:

- modified KDE CMake/QML/C++ files;
- new `kde/src/shortcutmanager.*`;
- new `kde/tests/qalc_test.cpp`;
- new transparent icons in `kde/resources/`;
- new/updated handoff docs.

Do not discard local changes unless the user explicitly asks.

## Known Follow-Ups

- Manually validate KWin keep-above rule on an actual KDE Wayland session after toggling the setting.
- Consider using KConfig if the project later adds KF6Config; current KWin rule writer is manual to avoid `QSettings` escaping `[General]`.
- Add packaged-install smoke test so desktop file/app id/portal behavior is tested after install, not only from the dev binary.
- Expand native tests for `DocumentModel::setKeepAbove()` with a temp config path if the implementation is refactored to inject config location.
