# Changelog

## 0.1.85 - 2026-09-05

### Added

- **Autocomplete for units, currencies and functions.** From two typed characters Tab offers libqalculate's units (`kilo` → kilometer, kilogram …), currencies (`US` → USD) and functions with an opening parenthesis (`sq` → `sqrt(`), each with its description. User variables and keywords keep coming first; at most 12 library names per group.
- **Error explanations.** Hovering a red `Error` shows why the line failed: division by zero, temperature below absolute zero, vectors and matrices, infinite or undefined results, or libqalculate's own message (for example a function called with too few arguments).

## 0.1.84 - 2026-09-05

### Fix

- **Complex results** are printed as libqalculate writes them (`sqrt(-1)` → `i`, `sqrt(-4)` → `2i`) instead of a bogus `0`.
- **Scientific notation** for magnitudes a double cannot show exactly (≥ 1e15: `6.022e23 * 2` → `1.204e24`, `2^100` → `1.268e30`) and for non-zero values that would otherwise vanish into `0` at the chosen precision (`1e-7` → `1e-7`, `1 / 3e10` → `3.333e-11`). At 0 decimals small values still round to `0`, as requested.
- **Cancelling a running evaluation no longer calls `Calculator::abort()`** from another thread: it raced with libqalculate's own timeout handling and could hang the calculation thread on slow machines. The superseded document still skips its remaining lines; the line in flight ends by the engine's per-line timeout.
- **Rounding is half-away-from-zero at every precision**: `2.5` → `3`, `-2.5` → `-3`, `1.005 * 100` → `101` at 0 decimals; `0.125` → `0.13`, `2.675` → `2.68`, `-1.005` → `-1.01` at 2 decimals. Previously the platform's banker's rounding and binary representation made `.5` cases inconsistent.

## 0.1.83 - 2026-09-05

### Fix

- **`sum(…, x)` / `product(…, x)` crashed the app**: between evaluations the engine deleted every non-builtin variable, including libqalculate's global unknowns `x`, `y`, `z`, leaving dangling pointers inside the library. Only variables created by the document are removed now. Found by the new golden tests.
- **Abort skips the rest of the document**: when new input supersedes a running evaluation, the lines not yet reached are no longer evaluated; the engine is destroyed safely even mid-calculation.
- **Tray icon for light colour schemes**: the original badge is shipped in an inverted variant (dark glyph on a light disc) and chosen from the Plasma colour scheme at start and whenever the scheme changes.

### Tests and CI

- **KWin script contract test**: `numi-kde-window-memory` runs in a QJSEngine with a mocked KWin `workspace` and `callDBus`; checks which windows it touches, what it reports, how it restores and when it must stay silent (off-screen, popups, foreign apps).
- **D-Bus contract test**: service, path and interface are parsed from `main.js`, validated against the D-Bus spec and the C++ `Q_CLASSINFO`, and round-tripped through a real session bus (`dbus-run-session` in CI). A hyphen in the interface name would now fail the build instead of silently breaking position memory.
- **Golden documents**: `kde/tests/fixtures/golden/*.numi` evaluated line by line against `.expected` (rounding at 0/2/10 places, leap-year and month-end dates, unicode variable names, currency, units, time, totals, error and incomplete-input cases). `numi-kde-tests golden --record` regenerates them.
- **Offline, deterministic exchange rates** for every test via `kde/tests/fixtures/rates.json` and `NUMI_KDE_OFFLINE_RATES`.
- **Engine stress test**: a long evaluation must stop promptly on `abortCalculation()`; completions, highlighting and setters are hammered from another thread while documents evaluate. A ThreadSanitizer CI job runs the engine and KWin suites with `-DNUMI_KDE_TSAN=ON`.
- **Install helper tests** (bash, stubbed `curl`/`rpm`/`dnf`): path arguments, bad versions, downgrades, missing assets, checksum mismatches and foreign package names never reach `dnf`.
- **RPM contents check** on every PR and release: all runtime files (desktop, metainfo, notifyrc, polkit policy, KWin script, all icon sizes) and dependencies must be present; `rpmlint` runs informationally.
- **End-to-end script** `scripts/e2e-window-position.sh` for a live Plasma session: show, move, hide, show, restart, compare geometry.

## 0.1.82 - 2026-09-05

### Fix

- **Original icons are back**: the SVG app icon and the symbolic tray icon introduced in 0.1.81 are removed. The original Numi-KDE logo is now shipped as PNG in all hicolor sizes (16–256 px) under the new icon name, and the tray uses the original monochrome badge again.

## 0.1.81 - 2026-09-05

### Fix

- **Window position on Wayland**: two problems. (1) The KWin rule was written in the KWin 5 `count=` format and the whole `kwinrulesrc` was rewritten on every start, which left KWin with an empty rule list — no keep-above at all. Rules are now managed through KConfig in the format KWin 6 reads (`[General] rules=` / `Order=`, UUID groups), only our own keys are touched, and the file is written only when something changed. (2) KWin never persists a "Remember position" rule for Wayland windows, so even a correct rule cannot bring the window back. Position memory is now done by a small KWin script shipped with the package (`numi-kde-window-memory`): it reports the geometry to the app over D-Bus when a window is moved or hidden and puts the window back when it appears. Legacy rule and autostart entries from earlier versions are migrated on first start.
- **Separate rule for the settings window**, exact `app_id` match instead of substring; the settings window position is remembered independently.
- **Maximized size no longer saved** as the normal window size.
- **Thread safety in the calculation engine**: autocomplete, help rendering and the settings setters no longer touch libqalculate state while a background evaluation is running (snapshot for completions, mutex in the remaining readers).
- **Running calculation is aborted** when new input arrives instead of finishing in the background.
- **Results column scroll sync** was bound to a `TextEdit` without a `contentY`; it now follows the editor's real Flickable.
- **Hourly rate refresh** updates the existing currency units in place instead of allocating new ones.
- Tray/QML warnings on start removed; unused `HelpPane.qml` deleted.

### Update flow

- **Privileged helper takes a version number, not a file path**: it downloads the RPM and `SHA256SUMS` from the GitHub release itself, verifies the checksum and the package name and refuses downgrades. Nothing writable by the user session is installed anymore.
- **Silent install, restart when idle**: the update is installed in the background without hiding the window. If the window is hidden the app restarts immediately (`--hidden`); otherwise it restarts on the next hide, with a notification offering "Restart now". A single "Numi-KDE updated" notification is shown after the restart.
- Failed installs can be retried; the setting is now "Install updates automatically".

### KDE / Linux integration

- **Single instance** via `KDBusService`: a second launch shows the existing window.
- **Reverse-DNS application id** `online.skorin.numi-kde` for the desktop file, AppStream metadata, icons and Wayland `app_id`.
- **KNotification** with a `notifyrc` for update events (configurable in System Settings).
- **Standard KDE shortcut recorder** (`KeySequenceItem`) in Settings; window activation via `KWindowSystem::activateWindow`; component shown as "Numi-KDE" in Shortcuts settings.
- **Esc hides the window** (new setting, on by default).
- Autostart entry written under `$XDG_CONFIG_HOME` with a relative `Exec`.
- AppStream metadata gained releases, screenshot, content rating and keywords; `desktop-file-validate` and `appstreamcli validate` run in CI.
- Semgrep uses the project's own rules only (no online registry) in the build and pre-commit hook.
- RPM now requires `kf6-kconfig`, `kf6-kdbusaddons`, `kf6-knotifications`, `kf6-qqc2-desktop-style`, `kf6-kdeclarative` and `curl`.

## 0.1.80 - 2026-09-05

### CI

- **GitHub Actions on Node.js 24**: upgraded `actions/checkout` (v7.0.1), `actions/upload-artifact` (v7.0.1), `actions/download-artifact` (v8.0.1) and `softprops/action-gh-release` (v3.0.3) ahead of the Node.js 20 runner removal on 2026-09-16
- **Actions pinned to commit SHAs**: all `uses:` references now point to full 40-character commit hashes (supply-chain hardening); this also unblocks the local `semgrep-check` build step, which flagged mutable tags

## 0.1.79 - 2026-08-06

### Fix

- **Auto-update on Fedora 44**: DNF5 rejected the `--` end-of-options separator in `dnf install -y --nogpgcheck -- <rpm>`, causing "Unknown argument" error. Removed `--` — the RPM path is always an absolute path and cannot be confused with an option flag.

## 0.1.78 - 2026-08-06

### Fix

- **Memory leak in update checker**: `prevUpdateState` was allocated with `new` and never freed; replaced with `std::shared_ptr` for proper lifetime management
- **Invalid temperature below absolute zero**: expressions like `-300 K to C` now correctly return an error instead of a nonsensical result; physical lower bound (0 K = −273.15 °C) is enforced
- **API error logging**: currency rate fetch failures now emit a `qWarning` message with details, making network issues easier to diagnose

## 0.1.77 - 2026-06-11

### Fix

- **2-digit years in date expressions**: `05.03.26` is now treated as 2026 (not 1926). All separators supported: `.`, `/`, `-`. Both `DD.MM.YY` and `DD.MM.YYYY` work interchangeably.

## 0.1.76 - 2026-06-11

### Fix

- **Auto-update now works without a password prompt**: replaced PackageKit Qt6 D-Bus API with a dedicated polkit helper (`/usr/libexec/numi-kde-install-update`). The helper is invoked via `pkexec` under its own polkit action (`online.skorin.numi-kde.update`, `allow_active=yes`) — active desktop sessions install updates silently with no dialog. Eliminates "Failed to obtain authentication" caused by the PackageKit daemon → D-Bus → polkit chain failing on KDE/Wayland.

## 0.1.75 - 2026-06-11

### Chore

- Test release to verify PackageKit Qt6 auto-update flow end-to-end

## 0.1.74 - 2026-06-11

### Fix

- **Auto-update now reliably installs**: replaced `pkcon` subprocess with PackageKit Qt6 D-Bus API (`PackageKit::Daemon::installFile`). The old approach silently failed on unsigned GitHub release RPMs; polkit authentication is now handled natively by the KDE polkit agent. Added tray notifications for download start and download failure (previously both were invisible to the user). Install error messages now include PackageKit error details.

## 0.1.73 - 2026-06-11

### Fix

- **Total footer now works for mixed currency expressions**: when a document combines plain amounts like `23.74 EUR` with conversions like `298.81 AED to EUR`, the total was silently hidden even though all results were in the same currency. Root cause: the `totalKey` used for grouping was derived from the *input expression* text rather than the *result* currency — plain `X EUR` got key `"number"` while `X AED to EUR` got key `"EUR"`. Fixed in all three custom currency evaluation paths (`tryEvaluateCurrencyExpr`, `currencyArithRegex`, `crossCurrencyRegex`).

## 0.1.72 - 2026-06-06

### Feature

- **Instant currency on startup**: exchange rates are cached to disk (`~/.cache/numi-kde/rates.json`) after each successful API response and restored before any network request — currency expressions evaluate correctly from the first keystroke even when offline or before the API replies

### Build

- Semgrep now scans all tracked files (not just C++) using community rules (`--config auto`) in addition to project rules
- Semgrep runs as a mandatory CMake build dependency (`semgrep-check` target) — the build fails if any rule fires

## 0.1.71 - 2026-05-17

### Feature

- **Silent auto-install**: update is installed automatically as soon as the RPM finishes downloading — no tray menu interaction needed; main window hides so any polkit dialog stays visible; on failure the tray shows a retry action

## 0.1.70 - 2026-05-17

### Fix

- **Editor focus on show**: keyboard focus goes to the editor automatically every time the window is shown — no mouse click required to start typing after toggling via hotkey or tray

## 0.1.69 - 2026-05-17

### Fix

- **Update install dialog now visible**: main window hides to tray before pkcon starts, so the system authentication (polkit) dialog is no longer obscured by the keep-above window — user sees the dialog and can confirm it immediately
- **Install action feedback**: action is disabled and renamed to "Installing…" while pkcon runs; on failure it re-enables as "Install Update (retry)…" so the user can retry without restarting
- **Install notification**: tray notification now explains that a system authentication dialog requires confirmation

## 0.1.68 - 2026-05-17

### Fix

- **Cursor color independent of KDE theme**: replaced `Controls.TextArea` with `TextEdit` in the editor pane — the cursor now always renders as yellow (`#ffd35a`) regardless of whether the KDE system theme is set to dark or light Breeze

## 0.1.67 - 2026-05-13

### Feature

- **Periodic update check**: app now checks for updates every hour (with the existing 24 h gate), so long-running sessions on systems that are never restarted will still receive update notifications automatically

### Fix

- **Flaky CI test**: increased `spy.wait` timeout from 2 s to 5 s in `documentmodel` test suite — prevents false failures on slow CI runners when evaluating currency expressions

## 0.1.66 - 2026-05-13

### Feature

- **Auto-download updates**: new "Auto-download updates" checkbox in Settings; when enabled, the app silently downloads the RPM in the background once a newer version is detected on GitHub
- **Tray install flow**: after download completes, a tray notification appears with an "Install Update" action that runs `pkcon install-local` (PackageKit polkit dialog); after successful install a "Restart to Apply Update" action is shown
- **Tray one-click install**: clicking the tray balloon message also triggers install; clicking the second balloon triggers restart

## 0.1.65 - 2026-05-13

### Fix

- **Settings window**: opens at natural content size; cannot be resized smaller than content in either axis
- **Tray menu**: global shortcut no longer shown twice — only the standard gray hint remains

## 0.1.64 - 2026-05-13

### UX

- **Settings**: replaced custom side panel with a native KDE window — standard title bar, window controls, KDE color theme; SpinBox controls for numeric settings; remembers size and position across sessions

## 0.1.63 - 2026-05-13

### UX

- **Settings panel**: replaced slide-in drawer with floating popup overlay — panel no longer pushes editor content aside; closes on click-outside or Escape; smooth fade-in/out animation; scrollbar restored (appears when window is small)

## 0.1.62 - 2026-05-13

### Fix

- **Trailing zeros**: integer results no longer show unnecessary decimal places — `100/20` shows `5` instead of `5.00`; fractional trailing zeros also stripped (`52.50` → `52.5`); temperature conversion follows the same rule (`212.00 °F` → `212 °F`)

### UX

- **Settings panel**: replaced sliders with compact numeric input fields (label above, field + hint below); panel width reduced to 176 px; header/footer margins tightened; scrollbar hidden (wheel scroll still works)

## 0.1.61 - 2026-05-13

### Fix

- **Async evaluation**: generation ID prevents stale `QtConcurrent` results from overwriting the latest evaluation when source changes rapidly
- **ShortcutManager**: `registerShortcut()` now returns `false` when `KGlobalAccel` is not compiled in, instead of silently claiming success
- **UpdateChecker**: `lastCheck` timestamp is written only after a successful HTTP 200 response; previously written before the request, causing a 24 h delay when offline
- **kwinrulesrc**: `QSaveFile::commit()` failure is now logged via `qWarning()` instead of silently ignored
- **setDecimalPlaces**: value is clamped to `[0, 10]` at C++ level, not only in the QML slider, preventing excessive formatting from a corrupt settings file
- **Variable name collision**: `monthly income` and `monthly_income` (and any other pair that normalises to the same internal name) now both produce an error instead of silently overwriting each other
- **Percent parser**: comma accepted as decimal separator (`10,5% of 200 = 21`); integer-fraction form avoids locale-sensitive libqalculate output
- **Currency parser — scientific notation**: `1e-3 BTC to USD`, `1e-3 BTC + 1e-3 ETH` and similar expressions now handled correctly; the exponent sign was previously misread as an arithmetic operator
- **Network status**: `m_fiatStatus` / `m_cryptoStatus` tracked independently; new `Partial` state shown in UI when only one source is available

### CI

- Added `.github/workflows/ci.yml`: build + test + Semgrep + ShellCheck on push/PR to `main`
- Release workflow pinned to `fedora:44` instead of `fedora:latest`
- `install.sh`: checksum grep uses `-F` with two-space prefix to match SHA256SUMS format exactly

### Docs

- README and `CLAUDE.md`: corrected C++17 → C++20

## 0.1.60 - 2026-05-13

### Fix

- **Temperature conversion**: `100 C to F`, `0 C to K`, `212 F to C`, `273 K to C` and all six C/F/K pairs now work correctly; previously libqalculate parsed `C` as Coulomb and `F` as Farad and returned garbage

### Docs

- `/help` section updated: added **Temp** row with all three scales, **Time** row (`time + 60 min`, `time - 2 h`), expanded **Dates** row with `now + 2 days` and `tomorrow + 5 days`; fixed broken `°C to F` example in **Units** row

## 0.1.59 - 2026-05-13

### Fix

- **Named-date arithmetic**: `today/now/tomorrow/yesterday ± N days/weeks/months/years` now returns `DD.MM.YYYY`; previously fell through to libqalculate which returned `2 d` or similar junk

## 0.1.58 - 2026-05-13

### Fix

- **`time + N unit`**: expressions like `time + 60 min` or `time - 30 s` now compute the correct result (current time ± offset → `HH:mm:ss`); previously `time` in arithmetic context fell through to libqalculate which returned garbage (`60.00 min + 0.86`)
- **Tab on time/unit words**: `seconds`, `minutes`, `hours` added to the Tab-completion keyword list; `260 seconds` + Tab now correctly completes `seconds`
- **Multi-word variable Tab**: typing the first word(s) of a multi-word variable followed by a space and Tab now completes the full variable name (e.g. `Var name ` + Tab → `Var name extended`); previously the trailing-space prefix yielded an empty match and Tab did nothing
- **Compound unit `.00`**: results like `4.00 min + 20.00 s` now display as `4 min + 20 s` — libqalculate `print()` no longer forces `min_decimals` for non-numeric compound expressions

## 0.1.57 - 2026-05-13

### Fix

- **Color scheme** — matches original Numi palette:
  - User variable names: **light blue** `#6bc5f8` (was orange `#fb923c`)
  - Comment lines (`#...`): **amber** `#e09030` (was blue `#6bc5f8`)
  - Math operators (`+`, `-`, `=`, `*`, `/`, `%`, `(`, `)`): **default white** — no span (was yellow)
  - Keywords (`to`, `as`, `today`, etc.) and units/currencies (`km`, `USD`): **yellow** `#ffd35a` (unchanged)

## 0.1.56 - 2026-05-12

### Fix

- **Comment color**: `#` comment lines now use the same orange `#fb923c` as user variables instead of cyan `#22d3ee`
- **Tab autocomplete prefix**: `getCompletions` and `_getWordStart` now extract only the last word when the segment after the operator contains spaces (e.g. `today - 30 da` → prefix `da` → completes to `days`); previously "30 da" was used as prefix and matched nothing, or "30 da" was deleted instead of just "da"

## 0.1.55 - 2026-05-12

### Fix

- **Probe check**: `--probe` now compares `numericValue == 4.0` instead of `result == "4"` — was failing since v0.1.53 when `smartFormat` started appending decimal places to all results

## 0.1.54 - 2026-05-12

### Fix

- **Tab autocomplete**: `_getWordStart()` returned absolute position 0 when no operator was found on the current line (`sepIdx = -1 → start = 0`) — `editor.remove(0, cursor)` erased all preceding content; fixed to use `lineStart` as fallback
- **Cursor drift**: `lineH` now derived from `editor.cursorRectangle.height` (TextArea's actual internal line height) instead of `fontMetrics.lineSpacing` — eliminates accumulated vertical drift between highlighted text and the cursor over multiple lines
- **Variable highlight color**: changed from `#f0c040` (gold, visually identical to operators) to `#fb923c` (orange), matching the original Numi palette distinction between user variables and operator keywords

## 0.1.53 - 2026-05-12

### Fix

- **Decimal places in results**: `smartFormat` now uses `locale.toString(value, 'f', maxDecimals)` without stripping trailing zeros — all numeric results show exactly the configured number of decimal places, consistent with the Total footer
- **libqalculate output precision**: set `po.min_decimals = m_decimalPlaces` and `po.use_min_decimals = true` so unit conversion and other non-numeric results also respect the decimal places setting

## 0.1.52 - 2026-05-12

### Fix

- **Cursor**: replaced custom Item+Rectangle with `Rectangle { height: editor.cursorRectangle.height }` — cursor now uses Qt's actual line height, eliminating cumulative drift on later lines
- **Total formatting**: switched from `parseFloat(...toFixed())` to `Number.toLocaleString(Qt.locale(), 'f', places)` — Total now shows thousands separator and respects the decimal places setting
- **Total copy**: same fix applied to clipboard copy on Total click

## 0.1.51 - 2026-05-12

### Fix

- **Variable color**: changed from orange (`#fb923c`) to yellow (`#f0c040`) to match original Numi palette
- **Variable highlight without value**: `Переменная =` (no RHS yet) now immediately highlights the name in variable color; pre-scan uses a looser regex that doesn't require a value after `=`
- **Labeled assignment**: `X = (annotation) = value` pattern now correctly evaluates to the numeric expression; the parenthesized annotation prefix is stripped before evaluation — matches Numi's behavior
- **Cursor centering**: cursor bar now anchored to line top instead of vertically centered within lineSpacing, fixing the downward offset caused by trailing leading space

### Tests

- 103 tests (82 QalcBridge + 14 DocumentModel + 7 Editor): added `labeled assignment` test for `(annotation) = value` pattern

## 0.1.50 - 2026-05-12

### Features

- **Multi-word variables**: variable names can now contain spaces, e.g. `monthly income = 5000`.
  Internally mapped to underscore names for libqalculate; display names preserved in UI and autocomplete.
- **Autocomplete redesign**: Tab completion now shows only user-defined variables (with their last
  computed value) and date keywords — no more irrelevant built-in functions. Popup width increased to 300px,
  descriptions shown in muted colour next to each name.
- **Syntax highlighter**: multi-word variable names highlighted correctly (longest-first matching);
  variable colour changed to orange (`#fb923c`).
- **Line context**: autocomplete engine receives the full line up to the cursor and extracts the
  current token after the last operator — enables completing `sin(myVar` → `myVariable`.

### Tests

- 102 tests (81 QalcBridge + 14 DocumentModel + 7 Editor): `getCompletions` tests rewritten to match
  new API — keyword prefix matching, user-defined variable discovery, line-context extraction, tab-format values.

## 0.1.49 - 2026-05-10

### Tests

- Added 31 new tests (101 total): `getCompletions` coverage for QalcBridge and
  DocumentModel (prefix match, case-insensitive crypto tickers, max 12 results,
  alphabetical sort, identifier-only output); new `editor` suite via QQuickView
  with mock model — regression test for Enter key, Tab completion queries, Esc.
- CMake: `editor-keyhandlers` added as a CTest target with offscreen platform.

## 0.1.48 - 2026-05-09

### Fix

- **Editor**: Enter key was consumed by the autocomplete popup handler even when the
  popup was closed, breaking new-line input. Fixed by explicitly passing the event
  through when popup is not visible.

## 0.1.47 - 2026-05-09

### UI

- **Help overlay**: `?` button at bottom-left toggles an inline help overlay covering
  the editor area. Button is ~15% smaller; turns blue while overlay is active.
  No more `/help` placeholder text — cleaner empty state on launch.
  Improved examples: temperature (`100 °C to F`), percent with add (`500 + 20%`),
  variable chaining (`price = 1200   tax = price * 0.2   price + tax`).
- **Tab autocomplete popup**: Tab now shows a dropdown list of completions with keyboard
  navigation (Tab/↓ = next, ↑ = prev, Enter = confirm, Esc = restore original word).
  Single-match still completes silently. Case-insensitive prefix matching (USD, EUR, BTC
  now complete from lowercase "usd", "eur", "btc").

## 0.1.45 - 2026-05-09

### Security

- **Update checker**: validate release URL from GitHub API before opening — must start
  with `https://github.com/DMYTROSKORIN/numi-kde` to prevent opening unexpected URLs.

### Docs

- Added `CLAUDE.md` — structural description of the project for AI agents.
- `README.md`: document CMake ≥ 3.24 build requirement.

## 0.1.44 - 2026-05-09

### UI

- **Settings panel**: version label is now a clickable link to the GitHub project page;
  turns blue and underlined on hover (acts as About).

## 0.1.43 - 2026-05-09

### Fixes

- **History — Ctrl+N**: current session is now saved to history before the document
  is cleared, so content typed before Ctrl+N is no longer lost.
- **History — hide window**: current session is now saved when the window is hidden
  (hotkey, `–` or `×` button), not only on app quit or opening the history panel.

## 0.1.42 - 2026-05-09

### UI

- **Settings panel**: reduced width from 380 px to 210 px; shortened "Result column width"
  label to "Result width"; disabled horizontal scrolling in the settings scroll view.

## 0.1.41 - 2026-05-09

### UI

- **Settings panel**: removed hint text below "Default currency" field; tooltip with the same
  hint is now shown on hover over the `?` icon next to the label — keeps the panel narrower.

## 0.1.40 - 2026-05-09

### Fixes

- **No-space currency parsing**: expressions like `1BTC`, `1.5ETH`, `USD100` now
  parse correctly — regex changed from `\s+` to `\s*` in `numCurrRx`/`currNumRx`.

### UI

- **Animated cursor**: cursor is now accent-yellow, vertically centered
  (`height = ascent + descent`), with a smooth blink cycle — 500 ms pause →
  150 ms fade-out → 350 ms pause → 150 ms fade-in.

## 0.1.39 - 2026-05-08

### Fixes

- **Settings panel width**: the settings drawer now scales with the window width
  (`min(windowWidth, 380 px)`) instead of being fixed at 280 px.
- **Cursor alignment**: switched highlight-layer row height from `fontMetrics.height`
  to `fontMetrics.lineSpacing` so the HTML overlay and the plain-text cursor stay
  in sync across all font sizes. Cursor delegate now has an explicit `height` equal
  to `lineSpacing`.
- **Default currency for fiat mixes**: the default-currency setting now applies to
  all mixed-currency expressions (e.g. `4000 EUR + 500 USD + 34000 AED → USD`),
  not only to cross-crypto sums.
- **Operators without spaces**: currency expressions like `D+F` or `1 BTC+1 ETH`
  (operator not surrounded by spaces) are now parsed correctly.

## 0.1.38 - 2026-05-08

### Features

- **Default currency setting**: added a "Default currency" field in Settings. Controls
  the output currency for mixed-cryptocurrency expressions where no explicit target is
  given (e.g. `1 BTC + 1 ETH` → `USD` by default). Accepts any supported currency code
  (USD, EUR, UAH, …). Persisted across sessions.

## 0.1.37 - 2026-05-08

### Fixes

- **Currency variable arithmetic**: variables assigned a currency-valued result
  (e.g. `A = 500 AED to USD`) now carry a currency tag. Subsequent expressions
  correctly produce labelled currency results in all forms:
  - `A + 200` → `USD 325` (was `336.147` without the USD label)
  - `A + 200 USD` / `A + USD 200` → `USD 325` (was unsimplified `USD 200 + 136.147`)
  - `B + C` where both variables hold currency values → correct single-currency sum
- **Assignment RHS currency handling**: expressions like `A = 10 BTC to EUR` or
  `A = 1 BTC + 1 ETH` now go through the custom currency evaluator instead of
  libqalculate directly. `A = 10 BTC to EUR` no longer returns Error; multi-crypto
  sums no longer output a random fiat currency (EUR) chosen by libqalculate's
  internal unit registry.

## 0.1.36 - 2026-05-07

### Changes

- **RPM-only packaging**: removed DEB package generation. `CPACK_GENERATOR` is now `RPM`
  only. The DEB target was never published in CI; removing it eliminates a misleading
  mismatch between the installer script and actual release assets.
- **install.sh / uninstall.sh**: restricted to Fedora (`dnf`). Ubuntu/Debian branch removed.
  Users on other distributions are directed to build from source.
- **HTTP timeouts**: all outgoing network requests (Frankfurter, CoinGecko, GitHub Releases
  API) now have a 10-second transfer timeout via `QNetworkRequest::setTransferTimeout`.
- **User-Agent headers**: Frankfurter and CoinGecko requests now send
  `User-Agent: numi-kde/<version>`, consistent with the existing GitHub API request.
- **README**: added command-line flag reference (`--hidden`, `--version`, `--probe`,
  `--help`); updated install section to clarify Fedora-only RPM distribution; improved
  build-from-source instructions with per-distro dependency commands.
- **docs/release.md**: updated release checklist to match RPM-only workflow; added
  build artifacts policy and `git rm --cached` instructions.
- **.gitignore**: added `_CPack_Packages/`, `kde/build*/`, `*.rpm`, `*.deb`,
  `release-*/`, `SHA256SUMS` to prevent build artifacts from being committed.

## 0.1.35 - 2026-05-04

### Features

- **Hidden startup support**: added the `--hidden` command-line flag to start the application without showing the main window.
- **Improved autostart behavior**: the application now uses the `--hidden` flag when launched via the "Launch at login" feature, ensuring it starts minimized in the system tray.

## 0.1.34 - 2026-05-04

### UX & Interface

- **Rebranded to Numi-KDE**: updated the application name to "Numi-KDE" in the main window title, application menu, and metadata to ensure consistent naming across the system.

### Packaging

- **Lean Ubuntu DEB build**: reduced the Ubuntu container dependency installation with `--no-install-recommends` and non-PTY dpkg output to keep the required DEB release job reliable.

## 0.1.33 - 2026-05-04

### Packaging

- **Restore DEB release support**: the Ubuntu packaging job now runs in an Ubuntu 25.10 container where KF6 development packages are available, and DEB builds are required for publishing a release.
- **Debian runtime dependencies**: CPack now uses `dpkg-shlibdeps` so generated `.deb` packages include runtime library dependencies instead of an empty `Depends` field.

## 0.1.32 - 2026-05-04

### Changes

- **Frankfurter fiat rates**: fiat currency conversion now uses `https://api.frankfurter.dev/v2/rates?base=USD` as the managed rate provider. Rates are converted into a local USD-based map and applied to libqalculate currency units.
- **Unified currency conversion path**: fiat and crypto conversions now share the same USD-rate logic, so expressions such as `500 AED to USD`, `1 BTC to EUR`, and `500 EUR - 100 USD` use consistent conversion behavior.
- **Stable fallback**: libqalculate exchange rates are still loaded as currency unit definitions and fallback data, but live fiat rates are refreshed from Frankfurter on startup and then hourly.

## 0.1.31 - 2026-05-04

### Bug Fixes

- **Global hotkey regression after 0.1.27**: restored the stable 0.1.27 toggle behavior so a visible window hides and a hidden window is shown even when another application has focus. Removed the `KWindowSystem::activateWindow()` toggle path because KWin may reject explicit activation as focus stealing on Wayland.
- **Minimize button requiring two hotkey presses**: the custom `–` button now hides the window to tray instead of using compositor minimize. This avoids unreliable Wayland minimize state detection and makes the next hotkey press show the window immediately.

## 0.1.30 - 2026-05-04

### Bug Fixes

- **Global hotkey did not work when the window was not focused**: fixed a critical Wayland regression where the hotkey did not activate the window when another application had focus. Root cause: `win->requestActivate()` on Wayland requires an XDG activation token, which is not available when triggered through D-Bus (KGlobalAccel). `KWindowSystem::activateWindow()` is now used to handle the Wayland activation protocol.

## 0.1.29 - 2026-05-04

### Bug Fixes

- **Hotkey did not work after minimize (final fix)**: fixed the window toggle logic. Instead of unreliable `windowStates()`/`isExposed()` checks (both incorrect on Wayland), it now uses `isActive()`: the window is hidden only when it is visible and focused. A minimized window is never active, so the hotkey always restores it. Bonus: if the window is open behind another application, the hotkey brings it forward instead of hiding it.

## 0.1.28 - 2026-05-04

### Bug Fixes

- **Minimize + hotkey/tray requires two presses (Wayland fix)**: on Wayland, the compositor owns the minimize state and never sets `Qt::WindowMinimized`, so the previous fix (0.1.27) did not work. `QWindow::isExposed()` is now used because it correctly returns `false` for minimized windows on both X11 and Wayland.

## 0.1.27 - 2026-05-04

### Bug Fixes

- **Minimize + hotkey requires two presses**: Fixed an issue where pressing the global hotkey (or clicking the tray icon) after minimizing the window would hide it instead of restoring it. Root cause: a minimized window reports `isVisible() == true` in Qt, so the toggle logic treated it as "shown" and called `hide()`. The fix checks `windowState()` for `Qt::WindowMinimized` and restores the window in that case.

## 0.1.26 - 2026-05-04

### Bug Fixes

- **Currency Variable Arithmetic**: Fixed a regression where `A = 500 AED to USD` followed by `A + 200` showed the unsimplified expression `USD 136.xxx + 200.000` instead of computing the sum. The variable is now stored as a plain numeric value so that arithmetic with plain numbers works correctly.
- **No Trailing Zeros on Integers**: Fixed the related issue where the integer `200` was displayed as `200.000` in compound expressions involving currency-valued variables.

## 0.1.25 - 2026-05-04

### Bug Fixes

- **Variable Substitution Fix**: Resolved an issue where variables assigned with currency conversions (e.g., `A = 500 AED to USD`) resulted in unsimplified expressions when used in subsequent calculations. Variables now preserve their mathematical structure instead of being treated as plain text.

### UX & Interface

- **Version Display**: Added the application version information to the bottom of the settings drawer for better version tracking.

## 0.1.24 - 2026-05-03

### UX & Interface

- **Shadowing Units for Variables**: Allowed using unit names (like `A`, `B`, `C`, `m`) as variable names. This resolves the "Error" result when assigning conversion results to short variable names, as the engine now correctly shadows the units.
- **Improved Error Reporting**: The interface now displays specific error messages (like "Reserved name") instead of a generic "Error" text.

## 0.1.23 - 2026-05-03

### Bug Fixes

- **Fix QML Loading Crash**: Resolved a "Non-existent attached object" error caused by an unqualified ScrollBar property in SettingsPane.qml.
- **Improved UI Robustness**: Added null checks for Window.window attached properties in HistoryPane and SettingsPane to prevent runtime crashes during engine initialization.

## 0.1.22 - 2026-05-03

### UX & Interface

- **Final Fix for Currency Sums**: Resolved the "Error" result when adding mixed currencies (e.g., `600 AED + 400 USD`). The fix involves improved preprocessing of currency symbols and correct handling of variable assignments for unsimplified results.

## 0.1.21 - 2026-05-03

### UX & Interface

- **Settings Interface Restyling**: Completely restyled the settings panel to match the Numi-KDE custom dark theme. Replaced default Qt Quick Controls 2 delegates with custom-drawn CheckBoxes, Sliders, and TextFields to ensure visual consistency.

## 0.1.20 - 2026-05-03

### UX & Interface

- **New Settings Interface**: Migrated the settings window into a side drawer (similar to the History pane). This completely resolves window positioning issues on Wayland by keeping all interface elements within the main application window.
- **Improved Math Logic**: Fixed a bug where currency sums (e.g., `AED + USD`) resulted in an "Error". The engine now correctly handles and displays multi-term currency results.

## 0.1.19 - 2026-05-03

### Performance & Stability

- **Wayland Position Memory Fix**: Rewrote the window position logic to resolve the "centering every second show" bug on KDE Wayland. The application now uses an idempotent KWin rule injection strategy and disables client-side position restoration on Wayland to prevent conflicts with KWin's native `Remember` mechanism.
- **Settings Dialog Jumping Fix**: Disabled manual dialog centering on Wayland to prevent the settings window from jumping to the top-left corner due to unreliable client-side coordinates.

## 0.1.18 - 2026-05-03

### Performance & Stability

- **Window Position Persistence Fix**: The main window now saves position only after user-initiated movement or resize, so compositor remap placement cannot overwrite the last user position.
- **Settings Dialog Placement Fix**: The settings dialog is now a transient child of the main window and recenters over it when opened.
- **Repository Hygiene**: Added tracked release/authorship rules and a network-independent semgrep pre-commit hook.

## 0.1.17 - 2026-05-03

### Performance & Stability

- **Window Position Persistence Fix**: The main window now saves its position immediately before hiding and disables compositor-placement writes while hidden, preventing the saved position from being overwritten by a centered fallback after the next toggle.
- **Settings Dialog Placement**: The settings dialog now opens centered over the main application window instead of persisting its own position.

## 0.1.16 - 2026-05-03

### Performance & Stability

- **Window Position Startup Fix**: The main window is now shown from C++ after startup preparation, so QML no longer maps it before KWin rules are written. Position persistence is owned by QML on X11 and by KWin's `Remember` rule on Wayland.
- **Settings Window Position Persistence**: The settings dialog now restores and saves its own position.
- **Public Repository Cleanup**: Removed internal notes and stale documentation references.

## 0.1.15 - 2026-05-03

### Performance & Stability

- **Dark-Only UI**: Removed the light theme option and related theme state.
- **Cross-Currency Arithmetic**: Fixed expressions such as `500 EUR - 100 USD` so the result keeps the left-hand currency.
- **Global Shortcut Reliability**: Restored `setShortcut(NoAutoloading)` alongside native global shortcut registration.
- **Window Position Follow-Up**: Improved hide/show position handling; startup restoration is completed in `0.1.16`.

### Calculator

- **Expanded Help**: Added Area, Data, and Speed examples to `/help`.

## 0.1.13 - 2026-05-03

### Performance & Stability

- **Final Fix for Global Shortcuts**: Resolved the persistence issue where custom shortcuts reverted to defaults. The system now uses local configuration as the source of truth and forces `KGlobalAccel` to apply these settings on every launch, bypassing daemon auto-loading conflicts.
- **Robust Window Position Restoration**: Implemented an active position restoration logic in QML (`onVisibleChanged`). The window now forcefully re-applies its saved coordinates whenever it transitions from hidden to visible, countering compositor-level placement resets on Wayland and X11.
- **Improved Mathematical Logic**: Enhanced the calculation engine to reject unsimplified results (e.g., `USD 500 - 600`) as `Error`. This prevents confusing output when mixing incompatible units or using incorrect number formats, ensuring results are always mathematically sound.

### UX & Branding

- **Interactive Update Notifications**: System notifications for new versions are now actionable. Clicking a notification (or the tray menu entry) will immediately open the official GitHub releases page for instructions and downloads.

## 0.1.12 - 2026-05-03

### Performance & Stability

- **Emergency Fix: Window Visibility**: Reverted to using `hide()` instead of `showMinimized()` for window closing. This fixes a critical regression where the frameless window would become unreachable on some Wayland/X11 configurations. Implemented manual position restoration to ensure the window still remembers its last location.
- **Emergency Fix: Global Shortcuts**: Completely rewrote the `ShortcutManager` to fix a regression where shortcuts failed to register or persist. The system now uses a hybrid approach of local `QAction` shortcuts and native `KGlobalAccel` registration, ensuring reliable triggering and persistent user preferences across restarts.

## 0.1.11 - 2026-05-03

### Performance & Stability

- **Strict Input Noise Filter**: Implemented a heuristic to ignore arbitrary text and invalid commands (e.g., junk text or malformed `/help` calls). Only lines that contain digits, mathematical operators, assignments, or known symbols are now evaluated, keeping the result area clean from non-mathematical noise.
- **Improved Currency & Large Number Parsing**: Fixed a critical issue where large numbers with thousands separators were being interpreted as vector lists. Reimplemented group separator removal using a robust backwards-iteration logic that handles multiple separators (e.g., `EUR 7,486,332.414`) correctly.
- **Finalized Shortcut Persistence**: Resolved an issue where custom global shortcuts reset after application restart. The system now uses robust `QSettings` management combined with `KGlobalAccel::NoAutoloading` and explicit desktop file identification, ensuring user preferences are strictly honored by the KDE desktop.

### Calculator

- **Division Operator Alias**: Added support for `:` as a division operator (e.g., `2 : 4` now returns `0.5`), matching common notation in some regions.
- **Vector Rejection**: Evaluated results that produce vectors or matrices (often caused by illogical or comma-heavy input) are now explicitly flagged as `Error` instead of showing confusing bracketed lists.

## 0.1.10 - 2026-05-03

### Performance & Stability

- **Improved Window Position Persistence**: Switched from `hide()` to `showMinimized()` for the main window toggle. This preserves the surface mapping and compositor state, ensuring the window reappears exactly where the user left it on both Wayland and X11.
- **Enhanced Calculation Parsing**: Fixed an issue where numbers with thousands separators (e.g., `22,045.00`) were incorrectly parsed as lists by `libqalculate`. Pre-processing now safely strips locale-aware group separators from numbers before evaluation.
- **Robust Shortcut Persistence**: Refactored `ShortcutManager` to use native `KGlobalAccel` configuration. User-defined global shortcuts are now stored directly in KDE's settings (`kglobalshortcutsrc`), ensuring they are remembered across app restarts.

### UX & Branding

- **Light Theme**: Added a new "Light Theme" option in settings with a palette inspired by old paper/milky white (`#f2e9d0`). All UI components (settings, history, drawer) now dynamically adapt to the selected theme.
- **Update Checker Notifications**: Fixed a `QSettings` organization mismatch that prevented update checks from persisting. Added system tray notifications for manual update checks: "Checking for updates...", "You are using the latest version", and "A new version is available".

## 0.1.9 - 2026-05-03

### Performance & Stability

- **Async Evaluation**: Moved the calculation engine (`libqalculate`) to a background thread using `QtConcurrent`. The UI now remains perfectly responsive even during heavy computation or rapid typing.
- **Throttling/Debouncing**: Added a 50ms delay to evaluations to prevent CPU spikes during rapid text input.
- **Highlighter Optimization**: Implemented classification caching in `SyntaxHighlighter`. Repeated tokens (units, keywords) are now highlighted near-instantly without re-querying the math engine.
- **Variable Persistence Fix**: User-defined variables are now correctly cleared before each full document re-evaluation, preventing deleted lines from affecting future results.
- **ListView Optimization**: Refactored the QML highlight layer to use `ListView` instead of `Repeater`. Only visible lines are rendered, drastically improving performance for large documents.

### UX & Branding

- **Network Status Indicator**: Added a colored status dot in the title bar to show the state of cryptocurrency rate updates (Fetching, Success, Error). Includes detailed ToolTips.
- **Professional Branding**: Updated all application identifiers and metadata to use the formal `numi-kde` organization name.
- **Contact Info**: Updated official contact email to `dev@skorin.online`.
- **Atomic KWin Rules**: Switched to `QSaveFile` for updating `kwinrulesrc`, ensuring configuration safety on KDE Wayland.

### Calculator

- **Variable Validation**: Improved assignment logic to prevent overwriting of system units (like `m`, `kg`, `USD`) or reserved keywords.

## 0.1.8 - 2026-05-02

### Update checker

- Added automatic update checking via the GitHub Releases API (`api.github.com`).
- On startup, the app checks for a newer release once per 24 hours (last-check timestamp stored in `QSettings`).
- The tray menu gains a **Check for Updates** action for manual checks.
- When a newer version is found, the action label changes to **Update available: vX.Y.Z** and clicking it opens the GitHub release page in the browser.
- Repository is now public: `https://github.com/DMYTROSKORIN/numi-kde`.

## 0.1.7 - 2026-05-02

### Packaging

- Added CPack configuration to `kde/CMakeLists.txt` for `.rpm` (Fedora) and `.deb` (Debian/Ubuntu) generation.
- Added install rules for `LICENSE` and `NOTICE` to `share/doc/numi-kde`.
- Fedora runtime dependencies confirmed from live binary: `libqalculate`, `qt6-qtbase`, `qt6-qtbase-gui`, `qt6-qtdeclarative`, `kf6-kwindowsystem`, `kf6-kglobalaccel`.
- Local RPM built, installed, probed and removed cleanly on Fedora KDE.
- Added `packaging/install.sh`: distro detection via `/etc/os-release`, GitHub Releases download, SHA256 verification, `dnf`/`apt-get` install, `--probe` smoke test, `--dry-run` and `--help` flags, `NUMI_KDE_VERSION` pin.
- Added `packaging/uninstall.sh`: package manager detection, removal, optional `--purge` with explicit confirmation.
- Added `.github/workflows/release.yml`: builds RPM on Fedora container and DEB on Ubuntu runner on `v*` tag push, generates `SHA256SUMS`, publishes GitHub Release.

### Window behavior

- Window position is now remembered correctly across sessions and within a session. Fixed a race condition where the compositor's initial placement overwrote the saved position before `Component.onCompleted` could restore it.
- X button and Alt+F4 now hide the window instead of quitting the application. The app can only be quit from the system tray **Quit Numi-KDE** action.
- The app no longer appears in the KDE panel task manager (`NET::SkipTaskbar | NET::SkipPager` always applied, not only when "Always on top" is enabled).

### Calculator

- `X% from/of N CURRENCY` now preserves the original currency. Previously `21.75% from 3654 AED` returned `USD 217.02` because libqalculate normalises currency results to USD when multiplying by a dimensionless fraction. The pattern is now computed directly.

### Visual

- Comment lines (`# …`) are now colored neon cyan (`#22d3ee`) instead of operator yellow, making them visually distinct.
- Added project logo to README.

## 0.1.6 - 2026-05-02

- Released the project under the Apache License 2.0.
- Updated AppStream metadata from proprietary placeholder licensing to `Apache-2.0`.
- Removed the README logo image and added a clear license section.
- Bumped the native project version to `0.1.6`.

## 0.1.5 - 2026-05-02

- Reworked `README.md` into a public-facing project overview with status, features, build instructions, acknowledgements and contribution notes.
- Refined the one-line installer plan around GitHub Releases, `.deb`/`.rpm` packages, checksums and thin install/uninstall scripts.
- Added `numi-kde --version` and `numi-kde --probe` for package smoke tests.
- Updated CMake install rules to install the binary, desktop file, AppStream metadata and the app icon.
- Changed the desktop launcher to use the installed `org.skorin.numi-kde` icon.

## 0.1.4 - 2026-05-02

- Changed inline `/help` date examples to use `01.01.2000`.
- Kept editor syntax colors stable while selecting text by drawing selection over the shared highlight layer.
- Shortened date-span output to a compact English single-line format without end-date wording or negative spans.
- Added a dedicated one-line installer implementation plan for KDE Ubuntu, Fedora and Debian support.

## 0.1.3 - 2026-05-02

- Routed inline `/help` examples through the shared C++ syntax highlighter so currencies, units and operators are colored consistently with normal input.
- Changed manual crypto conversion results to keep the target prefix, for example `1 BTC to EUR` returns `EUR <value>`.
- Added explicit date-to-date spans such as `26.08.1983 - 02.05.2026`, returning an English calendar span and total day count.
- Added regression tests for shared help highlighting, target-prefixed crypto output and detailed date spans.
- Refreshed docs for the current native-only behavior and verification status.

## 0.1.2 - 2026-05-01

- Matched the result separator default color to the app divider color used by the Total row and settings controls.
- Added operator highlighting inside inline `/help` examples without changing the help layout.
- Changed Total semantics so totals are shown only when every numeric row has the same total key; mixed units/currencies hide Total.
- Removed agent-only documents from the repository.
- Refreshed documentation to describe compatible totals and the cleaned native-only project structure.

## 0.1.1 - 2026-05-01

- Changed `/help` from a panel/card overlay to inline editor help text.
- Made the result separator visibly present while keeping the lag-free drag behavior.
- Changed Total to appear only for compatible numeric rows; mixed units/currencies such as `500 AED to USD` plus `1 BTC to UAH` no longer show a total.
- Added `today` operator highlighting.
- Added date arithmetic for inputs such as `26.08.1983 + 42 years`.
- Added regression tests for the reported help, total, highlighting and date arithmetic cases.

## 0.1.0 - 2026-05-01

Release candidate for the native KDE runtime.

- Reworked `/help` into a readable in-app guide with examples for math, variables, units, currency, crypto, dates, percentages and controls.
- Added adaptive result-column sizing for long results.
- Improved result separator dragging by using page-space pointer coordinates and clamping the minimum width to the rendered result content.
- Added typed Total support for compatible converted unit/currency/crypto rows.
- Added explicit date-span support for inputs such as `today - 26.08.1983`.
- Added native tests for date spans, converted-result totals and numeric conversion metadata.
- Removed non-native files so the repository contains only the native KDE project.
- Removed stale includes from `DocumentModel`.

Verification:

```sh
cmake --build build/kde --target numi-kde numi-kde-tests
ctest --test-dir build/kde --output-on-failure
```
