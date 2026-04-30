# numi-kde Handoff

Last updated: 2026-05-01.

Last completed handoff commit: `fix: Phase 2.1 editor rendering polish and UX fixes`.

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

- `npm test`: 52/52 pass.
- CMake configure/build: pass.
- Native GUI starts without stderr output.
- `git status --short`: clean after the pushed commit.

## What Works Now

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
- result hover highlight;
- clicking a result copies its formatted value to clipboard;
- semantic syntax highlighting: `*`, `/`, `%`, units, currency-like uppercase tokens,
  natural operators — all in entity (blue) or operator (yellow).

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

## Next Task

Start with **Phase 2.2 Settings Panel expansion**.

Recommended first patch:

1. Expand the existing settings popup in `kde/qml/DocumentPage.qml` with:
   - font size slider;
   - result column width control.
2. Wire new settings to `Qt.Settings` for persistence.
3. Propagate font size and column width through EditorPane and ResultsPane.
4. Add/update tests in `test/kde-skeleton.test.js`.
5. Run `npm test`, CMake configure/build, then launch `./build/kde/numi-kde`.
6. Update this file, `docs/implementation-plan.md` and `docs/kde-native.md`.
7. Commit and push.
