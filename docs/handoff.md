# numi-kde Handoff

Last updated: 2026-04-30.

Last completed handoff commit: `Document project handoff state` at current `HEAD`.

Last functional GUI commit: `1b28e45 Wire native GUI live evaluation`.

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

The latest completed step is pushed to GitHub.

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
- `git status --short`: clean immediately after the pushed handoff commit.

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
- parser token ranges for highlighting.

Native GUI:

- Qt Quick shell in `kde/`;
- compact dark frameless Numi-like window;
- Linux/KDE controls, no macOS traffic-light decoration;
- top-left history button placeholder;
- top-right minimize, maximize/restore and close;
- bottom-left settings gear placeholder close to the corner;
- no bottom-right arrow;
- always-on-top enabled by default;
- resizable from edges and corners;
- window geometry persists between launches;
- left editor is active and editable;
- right result column updates live while typing;
- result hover highlight;
- clicking a result copies its formatted value to clipboard;
- first syntax highlighting layer for units, currency-like uppercase tokens, `%`, and natural
  operators such as `in`, `to`, `as`, `of`, `from`.

## Important Files

- `docs/implementation-plan.md`: authoritative roadmap and progress log.
- `docs/kde-native.md`: native GUI build/run/status notes.
- `kde/qml/Main.qml`: frameless window, controls, resize handles, saved geometry.
- `kde/qml/DocumentPage.qml`: left editor/right results layout and model wiring.
- `kde/qml/EditorPane.qml`: active editor plus syntax highlight overlay.
- `kde/qml/ResultsPane.qml`: right result list, hover highlight and copy signal.
- `kde/src/documentmodel.h`
- `kde/src/documentmodel.cpp`: C++ model exposed to QML.
- `src/gui/evaluate-document.js`: Node worker entry point for native bridge.
- `src/gui/adapter.js`: core-to-GUI view model.
- `src/gui/highlight.js`: syntax highlight classifier and HTML renderer.
- `test/gui.test.js`: GUI adapter/highlight tests.
- `test/kde-skeleton.test.js`: native QML/CMake structure tests.

## Known Limitations

- Native GUI currently calls the JS evaluator through a local Node process. This is acceptable for
  development, but final Linux packaging must either embed Node cleanly or move the backend to a
  native runtime.
- Syntax highlighting is early. It works, but editor polish is incomplete:
  caret alignment, highlight overlay line height, selection behavior and long-document performance
  still need work.
- `AED`/`USD` are highlighted as currency-like tokens, but real currency conversion is not
  implemented yet.
- History and settings buttons are placeholders.
- Settings UI does not exist yet, although geometry persistence is already implemented.
- KRunner, global shortcut, tray, file dialogs, `.numi` file association and packaging are not
  implemented.

## Next Task

Start with **Phase 2.1 Editor Rendering Polish**.

Recommended first patch:

1. Inspect `kde/qml/EditorPane.qml`.
2. Make highlighted overlay and editable `TextArea` share exact font metrics, padding and line
   spacing.
3. Verify typing, selecting text, scrolling and resizing still work.
4. Keep result column behavior unchanged.
5. Add/update tests in `test/kde-skeleton.test.js` and `test/gui.test.js` if the QML contract or
   highlighter changes.
6. Run `npm test`, CMake configure/build, then launch `./build/kde/numi-kde`.
7. Update this file, `docs/implementation-plan.md` and `docs/kde-native.md`.
8. Commit and push.

Do not start packaging, KRunner or live currencies before this GUI polish step. The editor is the
highest-risk part of the product experience.
