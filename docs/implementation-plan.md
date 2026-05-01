# numi-kde Implementation Plan

Last updated: 2026-05-01.

## Goal

`numi-kde` should be a KDE-native Numi-like document calculator:

- text document on the left, aligned results on the right;
- instant recalculation while typing;
- compact scratchpad window;
- native KDE integration for tray, global shortcut, clipboard, autostart and window behavior;
- strong math/unit/currency/date support through `libqalculate`;
- no Node.js or web-prototype code in the repository.

## Current State

Primary implementation: `kde/`.

Completed:

- Qt/QML frameless compact window.
- C++ backend through `DocumentModel`.
- `libqalculate` bridge in `QalcBridge`.
- C++ syntax highlighter.
- Settings window.
- History drawer.
- Result copy and Total copy.
- Total includes converted unit/currency/crypto numeric values.
- Readable `/help` guide.
- Adaptive result column width with separator clamped to visible content.
- Explicit date difference support such as `today - 26.08.1983`.
- App/tray transparent PNG resources.
- Tray menu.
- Global shortcut via `KGlobalAccel`, default `Ctrl+Alt+1`.
- Configurable global shortcut with KDE conflict handling.
- `Ctrl+N` clears current document.
- `Tab` completion.
- Launch-at-login.
- X11 keep-above through `KX11Extras`.
- KDE Wayland keep-above via managed KWin Window Rule.
- Native tests in `kde/tests/qalc_test.cpp`.

Current verification:

```sh
cmake --build build/kde --target numi-kde numi-kde-tests
ctest --test-dir build/kde --output-on-failure
./build/kde/numi-kde
```

Latest local result: build passes, CTest passes 2/2, offscreen dev GUI starts without QML warnings.

## Workflow Rules

- Check `git status --short` before editing.
- Do not discard local changes without explicit user instruction.
- Every behavior change should get a native test where practical.
- Run `cmake --build build/kde --target numi-kde numi-kde-tests`.
- Run `ctest --test-dir build/kde --output-on-failure`.
- Update docs when behavior changes.
- Do not reintroduce Node.js, JavaScript runtime code, or the old web prototype.

## Native Test Coverage

Current native tests cover:

- arithmetic;
- formatting and no unnecessary trailing zeros;
- percentages;
- incomplete expression suppression;
- division-by-zero `Error`;
- variables and assignment;
- comments/empty lines;
- `/help`;
- `time` and `now`;
- explicit date differences;
- unit conversion;
- lowercase fiat preprocessing;
- mocked crypto conversion;
- functions;
- totals, including converted unit rows;
- completion;
- DocumentModel history, total, autostart and `/help`.

## Phase Status

### Phase A: Native Runtime

Status: mostly complete.

Done:

- keep the repository scoped to the native KDE runtime;
- evaluate through `libqalculate`;
- keep one result row per source line;
- expose QML model roles for result, ok/error, diagnostics and highlighted HTML.

Remaining:

- improve diagnostic messages beyond compact `Error`;
- add source ranges for errors if the UI starts drawing underlines/tooltips.

### Phase B: KDE Integration

Status: active.

Done:

- tray icon/menu;
- transparent app/tray icons;
- global shortcut with KDE conflict checks;
- launch-at-login;
- clipboard workflows;
- X11 keep-above;
- KDE Wayland KWin-rule keep-above.

Remaining:

- manually validate KWin rule behavior on KDE Wayland;
- add installed-app smoke test for `.desktop` app id and portal behavior;
- consider migrating the KWin rule writer to KF6Config if/when `KF6Config` is added.

### Phase C: Calculator Compatibility

Status: active.

Done:

- arithmetic, variables, units, percentages, dates/time keywords;
- explicit `today - date` spans;
- `20% from/of`;
- incomplete input stays quiet;
- explicit invalid math shows `Error`;
- crypto conversion for top symbols using CoinGecko rates.

Remaining:

- decide how to display live rate timestamps/source;
- add offline/failed-rate UI state;
- broaden date/time compatibility;
- decide what unsupported natural-language phrases should do.

### Phase D: UX Polish

Status: active.

Done:

- settings window;
- result separator setting;
- faded vertical result separator;
- Total footer;
- converted-result totals;
- adaptive result width;
- history drawer;
- configurable font/result width/precision;
- `Ctrl+N`;
- readable `/help` guide.

Remaining:

- visual screenshot checks for KDE Wayland, X11, HiDPI and fractional scaling;
- revisit settings layout once more options are added;
- decide whether result click should copy immediately or show copied feedback.

### Phase E: Packaging and Release

Status: not started.

Needed:

- install rules for icons if needed outside Qt resources;
- desktop/appstream validation;
- package smoke test;
- Flatpak/AppImage/RPM decision;
- CI for CMake build and native tests.

## Next Recommended Work

1. Test "Always on top" on a real KDE Wayland session after toggling the setting.
2. If KWin only applies the rule on new windows, add a forced hide/show or clear user-facing behavior.
3. Add injectable config path or helper class for KWin rule writing, then unit-test it.
4. Add installed desktop-file smoke test.
5. Add visual screenshots for icon transparency and the result separator.
