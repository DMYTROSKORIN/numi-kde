# Architecture

Last updated: 2026-09-05 (v0.1.87).

## Product Shape

`numi-kde` is a document calculator for KDE Plasma:

- users type plain text lines;
- meaningful lines get aligned results;
- variables (single- or multi-word) can refer to earlier lines;
- the UI is a compact, frameless, always-on-top scratchpad toggled by a global shortcut, living in the system tray.

## Process Model

```text
numi-kde (GUI, Qt Quick)                     numi-kde-engine (/usr/libexec)
┌──────────────────────────────┐  stdin/stdout ┌──────────────────────────────┐
│ DocumentModel                │  JSON lines   │ EngineServer (main thread)   │
│   └─ EngineClient ───────────┼──────────────▶│   I/O, snapshot completions  │
│      watchdog, respawn,      │◀──────────────┤   └─ one-thread QThreadPool  │
│      poisoned lines          │  progress,    │        └─ QalcBridge         │
│ WindowMemory (D-Bus)         │  lines, events│             └─ libqalculate  │
│ UpdateChecker, ShortcutMgr   │               └──────────────────────────────┘
└──────────────────────────────┘
```

**Why two processes.** libqalculate's `Calculator` is a process-wide singleton whose calculation thread
cannot be stopped safely: a force-stopped thread hangs `~Calculator()` and process exit, and driving the
library from two threads (even serialised by a mutex) stalls it for the full per-line timeout on every
line. Hosting it in a child process turns every such failure into a restart the user barely notices.

**Threading contract** (`kde/src/qalcbridge.h`): everything that reaches the `Calculator` — evaluation,
highlighting, single-word completion, settings, rate application — runs on exactly one thread. In the
engine that is a `QThreadPool` with one thread; the main thread only does I/O and answers Tab completions
from a lock-free snapshot published after each evaluation.

**Failure handling** (`kde/src/engineclient.*`):

| Event | Reaction |
|-------|----------|
| engine exits / crashes | in-flight documents are reported (finished lines empty), engine respawned with backoff (150 ms … 5 s), document recomputed |
| no finished line for 6 s (`watchdogMs`) | engine killed; the line in flight is reported as *Calculation took too long* and **poisoned**: its exact text is sent as `skip` on later evaluations until the user edits it; the rest of the document is computed by the fresh engine |
| engine missing | every line reports *Calculation engine is not available*; nothing blocks |
| synchronous helpers (completions, highlight) | 400 ms cap, empty result on timeout — the GUI never waits on libqalculate |

Inside the engine libqalculate itself gets 3 s per line (`kLineTimeoutMs`); the GUI watchdog sits above that.

**Protocol** (`kde/src/engineprotocol.h`): one compact JSON object per line, UTF-8.

| Direction | Message |
|-----------|---------|
| GUI → engine | `configure {decimals, currency}`, `evaluate {source, skip[]}`, `cancel {target}`, `completions {context}`, `completion {prefix}`, `highlight {line}`, `ping`, `_crash` (tests) — all with an integer `id` |
| engine → GUI | replies carry the request `id`; `evaluate` streams `{ev:"progress", id, index}` per finished line, then `{id, lines:[LineResult…]}`; unsolicited `{ev:"ready", version}`, `{ev:"ratesUpdated"}`, `{ev:"networkStatus", value}` |

Request ids come from one counter for every message type so a `configure` reply can never be mistaken
for an evaluation reply. The engine is located via `$NUMI_KDE_ENGINE`, next to the GUI binary
(development builds), `<bindir>/../libexec`, the compile-time libexec dir, then `/usr/libexec`.

## Source Layout

1. `kde/src/engine/enginemain.cpp` — the engine process (`StdinReader` thread + `EngineServer`).
2. `kde/src/engineprotocol.*`, `kde/src/lineresult.h` — wire format and the one result struct shared by all sides.
3. `kde/src/engineclient.*` — GUI-side process owner (see above).
4. `kde/src/qalcbridge.*` — libqalculate bridge and compatibility preprocessing (engine process only).
5. `kde/src/syntaxhighlighter.*` — token-based HTML highlighter for the editor overlay (engine process only).
6. `kde/src/documentmodel.*` — QML model: document state, results, totals, history, clipboard, autostart, KWin rules.
7. `kde/src/kwinrulemanager.*`, `kde/src/windowmemory.*`, `kde/resources/kwin-script/` — Wayland window behaviour (below).
8. `kde/src/shortcutmanager.*` — KGlobalAccel registration; `kde/src/updatechecker.*` — self-update.
9. `kde/qml/` — `Main.qml` (window chrome, settings persistence), `DocumentPage.qml` (editor/results split, total footer, inline help), `EditorPane.qml` (TextEdit + highlight overlay + Tab completion popup), `ResultsPane.qml`, `SettingsWindow.qml`/`SettingsPane.qml`, `HistoryPane.qml`.

## Runtime Flow

```text
QML EditorPane.onTextChanged
  -> documentModel.source = text
  -> DocumentModel::setSource()            50 ms debounce, ++generation
  -> EngineClient::evaluate(generation)    cancel older in-flight ids, send evaluate (+ poisoned skip list)
  -> engine: QalcBridge::evaluateDocument  per line, progress event after each
  -> engine reply {lines}                  EngineClient::evaluated(generation, lines)
  -> DocumentModel::onEvaluated()          drops superseded generations, beginResetModel/endResetModel
  -> ResultsPane / EditorPane overlay      render result and highlighted HTML per row
```

`QalcBridge::evaluateDocument` per line:

1. blank / `#` comment → empty result;
2. superseded (`abortCalculation()` flag) or poisoned (`skipLines`) → skipped;
3. explicit division by zero → *Division by zero*;
4. `tryEvaluateCurrencyExpr()` — mixed-currency arithmetic and `X CUR to CUR` with Frankfurter/CoinGecko rates;
5. temperature conversion with the absolute-zero check;
6. `tryDateDifference()`, `tryDateArithmetic()`, `tryTimeArithmetic()` — dates as `DD.MM.YYYY` / `DD.MM.YY`, named dates, spans;
7. percent forms (`20% of 500`, `500 + 10%`);
8. assignments `Name = expr` / `Name := expr` (multi-word names normalised to underscores internally);
9. everything else → `libqalculate::calculate()` with a 3 s timeout.

Preprocessing also covers case-insensitive fiat symbols, incomplete-input suppression (`500/`, `600 USD to`)
and junk-text lines (no digits → no result, no error).

### Number formatting (`smartFormat`)

- rounding is half-away-from-zero at the configured precision (`2.5 → 3`, `-1.005 → -1.01`), with a tiny nudge
  so binary halves just below `.5` still round up;
- magnitudes ≥ 1e15 and non-zero values that would vanish into `0` at the chosen precision use scientific
  notation (`1.204e24`, `3.333e-11`); at 0 decimals small values do round to `0`;
- results with an imaginary part are printed by libqalculate (`sqrt(-1)` → `i`);
- digit grouping follows the default `QLocale`; libqalculate's own printing follows the C locale
  (`LC_ALL`), which is why tests pin both to `en_US`.

### Errors

Failed lines carry a reason (`LineResult::error`, QML role `diagnostic`, shown as a tooltip on the red *Error*):
division by zero, temperature below absolute zero, vectors/matrices, infinite or undefined result,
non-finite number, *Calculation took too long*, or libqalculate's own message (e.g. a function called with
too few arguments).

### Completions

`QalcBridge` keeps a name index (units, currencies including rate-created ones, functions with `(`),
rebuilt on the calculator thread whenever units change and read under its own mutex. `getCompletions()`
returns user variables first, then keywords, then from two typed characters currencies, units and
functions (≤ 12 per group). Common SI-prefixed long names (`kilometer`, `milligram`, `gigabyte`) are
synthesised because libqalculate stores prefixes apart from units.

`LineResult::hasNumericValue` and `LineResult::totalKey` are the boundary between display formatting and
totals: `DocumentModel` shows a total only when every numeric row shares one total key.

## KDE Integration

- **Application id**: `online.skorin.numi-kde` — desktop file, AppStream id, icon name, Wayland `app_id`
  (`QGuiApplication::setDesktopFileName`). `applicationName`/`organizationName` stay `numi-kde` so QSettings
  (`~/.config/numi-kde/numi-kde.conf`), the rates cache and the KGlobalAccel component keep their paths.
- **Single instance**: `KDBusService(Unique)` on `online.skorin.numi-kde`; a second launch shows the window
  (unless started with `--hidden`).
- **Global shortcut**: `KGlobalAccel`, default `Ctrl+Alt+1`, recorded with the standard `KeySequenceItem`;
  the window is activated through `KWindowSystem::activateWindow` so xdg-activation tokens are honoured.
- **Keep above / skip taskbar**: X11 via `KX11Extras`; Wayland via two managed KWin rules (main and settings
  window, exact `app_id` + title match) written through KConfig by `KWinRuleManager` in the KWin 6 format
  (`[General] rules=`/`Order=`, UUID groups), touching only our own keys, followed by `org.kde.KWin reconfigure`.
- **Window position on Wayland**: KWin never persists `positionrule=4` for Wayland windows, so the package
  ships a KWin script (`numi-kde-window-memory`, enabled by default). It reports geometry over D-Bus
  (`online.skorin.numi_kde.WindowMemory` at `/WindowMemory`, `kde/src/windowmemory.*`) when a window is moved
  or hidden and restores it on `workspace.windowAdded`; positions live in `QSettings [WindowMemory]`.
  D-Bus interface names cannot contain `-`, hence `numi_kde`.
- **Notifications**: `KNotification` events `updateInstalled`, `updated`, `updateFailed` from
  `kde/resources/numi-kde.notifyrc`.
- **Autostart**: `$XDG_CONFIG_HOME/autostart/online.skorin.numi-kde.desktop` with `Exec=numi-kde --hidden`.
- **Tray**: original monochrome badge; an inverted variant is picked for light colour schemes and swapped on
  palette change.
- **Esc** hides the window (setting, on by default).

## Self-Update

`UpdateChecker` polls `releases/latest` (hourly timer, 24 h gate). A newer version runs
`pkexec /usr/libexec/numi-kde-install-update <version>` (polkit action `online.skorin.numi-kde.update`,
`allow_active=yes`). The helper accepts only a version number: it downloads the RPM and `SHA256SUMS` from the
GitHub release into a root-owned directory, verifies the checksum and the package name, refuses downgrades,
imports `/etc/pki/rpm-gpg/RPM-GPG-KEY-numi-kde` and requires `rpmkeys --checksig` to report a valid
signature made by key `411c68b856cec16e`, then installs with `dnf`. Installation is silent; the app restarts
with `--hidden` as soon as its window is hidden (immediately if it already is), otherwise offers *Restart now*
in a notification. See `docs/release.md` for signing.

## Compatibility Strategy

Public Numi source code is not available in this repo. Compatibility is driven by:

- examples from public docs and user workflows;
- native tests in `kde/tests/qalc_test.cpp` and golden documents in `kde/tests/fixtures/golden/`;
- black-box comparisons against official Numi builds if available.

Behaviour is covered by tests before it is considered stable; `CLAUDE.md` lists every test layer.

## Design Constraints

- Keep calculation logic out of QML.
- Keep libqalculate out of the GUI process and on a single thread inside the engine.
- Keep the repository scoped to the native KDE application.
- Keep QML small and focused on layout/interaction.
- Prefer KDE-native integration points over ad hoc platform hacks.
- On Wayland, compositor-owned behaviour must go through supported KDE/KWin mechanisms.
