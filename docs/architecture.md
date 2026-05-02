# Architecture

Last updated: 2026-05-02.

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
- explicit date spans such as `today - 26.08.1983` and `26.08.1983 - 02.05.2026`;
- explicit date arithmetic such as `26.08.1983 + 42 years`;
- manual crypto conversion for top CoinGecko symbols.

Manual crypto conversion returns the target prefix with the formatted value, matching the rest of the conversion UI. For example, `1 BTC to EUR` displays as `EUR <value>`.

Date spans are formatted in English as a calendar period plus total days, for example:

```text
42 years, 8 months and 6 days (excluding the end date).
15,590 days.
```

`LineResult::hasNumericValue` and `LineResult::totalKey` are the boundary between display formatting and totals. `DocumentModel` sums only when every numeric result row has the same total key, so mixed units or currencies do not produce a misleading total.

Inline `/help` examples are plain strings in QML and are highlighted through the same C++ highlighter used for normal editor input. This keeps currencies, units and operators visually consistent across the app.

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
- Keep the repository scoped to the native KDE application.
- Keep QML small and focused on layout/interaction.
- Prefer KDE-native integration points over ad hoc platform hacks.
- On Wayland, compositor-owned behavior must go through supported KDE/KWin mechanisms.
