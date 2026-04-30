# numi-kde Handoff

Last updated: 2026-04-30.

Last completed handoff commit: `fix: Phase 2.1.1 cursor, hover and highlight fixes`.

Last functional GUI commit: current HEAD.

## User Goal

Build a KDE/Linux-native Numi clone with matching workflow, visual style and feature set.

GUI quality is P0. The application must feel like Numi: editable calculation document on the left,
results on the right, instant recalculation, syntax highlighting, compact always-available window,
and KDE-native behavior.

## Non-Negotiable Workflow Rules

- Every new behavior must have tests.
- Run `npm test` after every functional step.
- For native GUI changes, also run:
  - `cmake -S kde -B build/kde`
  - `cmake --build build/kde`
  - `./build/kde/numi-kde`
- Update `docs/implementation-plan.md` and relevant docs after each step.
- Commit and `git push` after green tests.
- Do not leave undocumented progress in the working tree.

## Current Verified State

Verified commands:

```sh
npm test
cmake -S kde -B build/kde
cmake --build build/kde
./build/kde/numi-kde
```

Expected status:

- `npm test`: 54/54 pass.
- CMake configure/build: pass.
- Native GUI starts without stderr output.
- `git status --short`: clean after the pushed commit.

## What Works Now (Phase 2.1.1)

Core:

- arithmetic precedence, parentheses, powers, unary operators;
- implicit multiplication;
- constants and common math functions;
- named variables and `:=` assignment;
- comments and blank lines;
- decimal comma and thousands separators;
- unit conversions for length, mass, time, data, CSS units and temperature;
- Numi-like percentage expressions;
- deterministic ISO date arithmetic;
- extension skeleton for variables/functions/units;
- diagnostics with source ranges;
- parser token ranges for highlighting;
- non-ASCII/Unicode input (Cyrillic etc.) handled gracefully — tokenised as identifiers,
  returns a diagnostic error instead of crashing the evaluator.

Native GUI:

- Qt Quick shell in `kde/`;
- compact dark frameless Numi-like window;
- Linux/KDE controls: top-left history button, top-right minimize/maximize/close;
- bottom-left settings gear — opens a Popup with "Всегда поверх окон" toggle;
- always-on-top saved in Qt Settings, default true, reflected in window flags at launch;
- resizable from edges and corners;
- window geometry persists between launches;
- empty field: cursor forced to position 0 when no text;
- cursor rendered via `cursorDelegate: Rectangle` (visible on dark background);
- highlight overlay and TextArea share `lineH = fontMetrics.height` for pixel alignment;
- placeholder text (grey) disappears on focus;
- result column has correct width (`width: root.availableWidth`);
- result hover: only result text turns bright yellow `#ffd35a`; click copies to clipboard;
- semantic syntax highlighting: `*`, `/`, `%`, units, currency-like uppercase tokens,
  natural operators — entity (blue) or operator (yellow);
- defined variable names highlighted neon green `#39ff14` everywhere they appear;
- `/help` command returns one-line feature summary in the result column;
- placeholder text changed to `/help` hint, dimmer color;
- always-on-top flag explicitly re-applied in `onAlwaysOnTopChanged` (works on X11/XWayland).

## Known Limitations

- Native GUI calls the JS evaluator through a local Node process. Final packaging must embed Node
  or move to a native runtime.
- Scroll sync between editor and results is wired (Connections block) but TextArea does not
  expose `contentY` directly — scrolling long documents may desync results column.
  Fix in Phase 2.3.
- Settings UI has only the "always on top" toggle. Font size and result column width are not yet
  configurable. Phase 2.2.
- History panel button is a placeholder.
- KRunner, global shortcut, tray, file dialogs, `.numi` association and packaging are not
  implemented.

## Important Files

- `docs/implementation-plan.md`: authoritative roadmap and progress log.
- `docs/kde-native.md`: native GUI build/run/status notes.
- `kde/qml/Main.qml`: frameless window, controls, dynamic flags, `alwaysOnTop` property.
- `kde/qml/DocumentPage.qml`: layout, settings popup with alwaysOnTop toggle.
- `kde/qml/EditorPane.qml`: editor with FontMetrics, `cursorDelegate`, placeholderText, overlay.
- `kde/qml/ResultsPane.qml`: result list with proper sizing, scroll sync Connections.
- `kde/src/documentmodel.h` / `documentmodel.cpp`: C++ model exposed to QML.
- `src/gui/evaluate-document.js`: Node worker entry point.
- `src/gui/highlight.js`: syntax highlight — `\p{L}` TOKEN_PATTERN, `*`/`/` as entity.
- `src/core/syntax.js`: tokenizer with `\p{L}` Unicode identifier support.
- `src/core/engine.js`: evaluator with try-catch in `evaluateLine` for robustness.
- `test/kde-skeleton.test.js`: native QML/CMake structure tests.

## Known Limitations (updated)

- cursor first-char bug is fixed (removed `onCursorPositionChanged` handler).
- always-on-top works on X11/XWayland; on native Wayland needs KWindowSystem `setKeepAbove()`.
- `/help` result is one short line; full help panel not yet implemented.
- variable neon green works for user-defined variables via `:=` or `=` assignments.
- currency conversion (`100 USD to AED`) still requires qalculate migration.
- `20% from 100` still requires qalculate migration.

## Next Task

Priority order:

1. **Phase 2.2** — Settings panel: font size slider + result column width.
2. **Phase 2.3** — libqalculate C++ migration (replaces Node worker; fixes currency, percentage,
   and all advanced math). See `docs/Gemini-qalculate-evolution.md` for the full plan.
3. **Wayland always-on-top** — use KWindowSystem `setKeepAbove(windowHandle(), value)` in C++.
