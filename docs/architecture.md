# Architecture

Last updated: 2026-05-01.

## Product Shape

`numi-kde` is a document calculator:

- users type plain text lines;
- meaningful lines get aligned results;
- variables can refer to earlier lines;
- the UI is a compact scratchpad rather than a traditional keypad calculator.

## Current Layers

1. `kde`: primary native KDE app, Qt/QML/C++.
2. `kde/src/qalcbridge.*`: `libqalculate` evaluation bridge and compatibility preprocessing.
3. `kde/src/documentmodel.*`: QML model, history, clipboard, settings-backed behavior, KWin integration.
4. `kde/src/syntaxhighlighter.*`: C++ semantic-ish highlighting for the editor overlay.
5. `kde/src/shortcutmanager.*`: KDE global shortcut registration and conflict handling.

## Native Runtime Flow

```text
QML EditorPane
  -> DocumentPage updates documentModel.source
  -> DocumentModel::evaluate()
  -> QalcBridge::evaluateDocument()
  -> libqalculate + preprocessing
  -> LineResult display text + optional numeric total value
  -> DocumentModel list roles and aggregate total
  -> ResultsPane / EditorPane overlay render rows
```

Preprocessing currently handles:

- case-insensitive fiat units where libqalculate has the unit;
- `20% from/of X`;
- incomplete input suppression;
- explicit division-by-zero error;
- `time` and `now`;
- explicit date spans such as `today - 26.08.1983`;
- manual crypto conversion for top CoinGecko symbols.

`LineResult::hasNumericValue` is the boundary between display formatting and totals. `DocumentModel` should sum this explicit numeric value instead of reparsing formatted result strings with units, currencies, or locale separators.

## KDE Integration Boundaries

- Tray and app icon: Qt resources from `kde/resources`.
- Global shortcut: `KGlobalAccel`, default `Ctrl+Alt+1`.
- X11 keep-above: `KX11Extras` / `NET::KeepAbove`.
- Wayland keep-above: managed KWin Window Rule in `kwinrulesrc` plus DBus `reconfigure`.
- Autostart: `~/.config/autostart/numi-kde.desktop`.
- Settings/history: `QSettings`, except KWin rules which are written manually to preserve KWin INI syntax.

## Compatibility Strategy

Public Numi source code is not available in this repo. Compatibility should be driven by:

- examples from public docs and user workflows;
- native tests in `kde/tests/qalc_test.cpp`;
- black-box comparisons against official Numi builds if available.

Behavior should be covered by tests before being considered stable.

## Design Constraints

- Keep calculation logic out of QML.
- Do not reintroduce Node.js or the old web prototype.
- Keep QML small and focused on layout/interaction.
- Prefer KDE-native integration points over ad hoc platform hacks.
- On Wayland, compositor-owned behavior must go through supported KDE/KWin mechanisms.
