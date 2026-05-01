# AI Handoff — numi-kde

Last updated: 2026-05-01 (v4 - Inline Help and Conversion Totals).

Read this before touching the repository.

## Repository

- Path: `/home/skorin/Git/numi-kde`
- Primary app: native KDE app in `kde/`
- Current native stack: Qt 6.10, QML, C++, `libqalculate`, KF6WindowSystem, KF6GlobalAccel when available
- Current shell: `bash`

## Current Status

The project is functionally complete and polished. The repository now contains only the native KDE/C++ application, resources, native tests, and current documentation.

Verified commands:

```sh
cd /home/skorin/Git/numi-kde
cmake --build build/kde --target numi-kde numi-kde-tests
ctest --test-dir build/kde --output-on-failure
./build/kde/numi-kde
```

Latest result:
- `numi-kde`: builds.
- `numi-kde-tests`: builds.
- `ctest`: 2/2 pass (includes locale-aware math tests).

## Important: Dirty Worktree

The worktree has active implementation changes. Do not reset or checkout files unless the user explicitly asks.
Recent updates (2026-05-01) include:
- Refined `Always on Top` logic (no window re-centering).
- Reworked `/help` into clear inline editor text, not a modal/card overlay.
- Result column auto-expands to fit long result text.
- Result separator drag uses page coordinates, clamps to the current visible result width, and remains visibly present.
- Locale-aware thousands separators in results.
- `Total` now includes numeric values from converted unit/currency/crypto results, including fiat-target crypto conversions.
- `today - DD.MM.YYYY` returns a year/day date span.
- `DD.MM.YYYY + N years/months/weeks/days` returns the resulting date.

## Key Technical Decisions

### Always on Top (KDE Wayland/X11)
- **Problem:** Dynamic window flags in QML cause window recreation and re-centering.
- **Solution:** Flags are now static in QML. All state management is in C++ (`DocumentModel::setKeepAbove`).
- **Wayland:** Uses `kwinrulesrc` with both `above=true/false` and `keepabove=true/false` with `aboverule=2` (Force) to ensure KWin applies/removes state instantly via DBus `reconfigure`.
- **X11:** Uses `KX11Extras` for `NET::KeepAbove` and `NET::SkipTaskbar`.

### UX Polish
- **Help Text:** Triggered by `/help`. It renders as inline mono text in the editor area below the typed `/help` line.
- **Result Width:** `DocumentPage.qml` measures rendered result text and total text, then uses that as the minimum result width. The window minimum width grows when needed for very long results.
- **Splitter:** Vertical line between editor and results. Dragging uses page-space coordinates, updates `Window.resultWidth`, and clamps to the measured visible content width.
- **Formatting:** `QalcBridge` uses `QLocale` for system-specific thousands separators. `libqalculate` is configured with `DIGIT_GROUPING_LOCALE`.
- **Totals:** `LineResult` carries an explicit numeric value when a result is numeric or a conversion result. `DocumentModel` uses that value instead of parsing display text. Display-number parsing finds numbers even after currency symbols.
- **Date Handling:** `QalcBridge` handles `today - DD.MM.YYYY` and `DD.MM.YYYY + N years/months/weeks/days` before passing expressions to `libqalculate`.

### Help Feature
- `/help` command in the editor triggers inline help text in `DocumentPage.qml`.
- `DocumentModel` clears the result role for `/help` so the right column stays clean.

## Files To Know

```text
kde/src/documentmodel.cpp    -> Window state (KWin rules, X11 extras), history, model logic.
kde/src/qalcbridge.cpp       -> libqalculate integration, locale-aware formatting.
kde/qml/Main.qml             -> Main window, static flags, settings bindings.
kde/qml/DocumentPage.qml     -> Editor/Results layout, Help overlay, Splitter logic.
kde/qml/EditorPane.qml       -> TextArea, Tab completion, subtle placeholder.
kde/tests/qalc_test.cpp      -> Native tests (including locale-aware expectations).
```

## Next Recommended Work

1. Add explicit "Copied to clipboard" visual feedback when clicking results.
2. Broaden natural language date support beyond explicit date difference (e.g., "next tuesday").
3. Implement a "Compact Mode" that hides the header/footer entirely.
4. Prepare Flatpak manifest for distribution.

## Do Not Regress

- **Do not change window flags dynamically in QML.** This causes re-centering.
- **Do not use `Qt.copyToClipboard`.** Use `documentModel.copyText()`.
- **Do not reintroduce Node.js, JavaScript runtime code, or the old web prototype.**
- **Do not make `/help` low contrast again.** It must remain readable and useful as a real instruction panel.
