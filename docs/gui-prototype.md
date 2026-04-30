# GUI Prototype

The first GUI prototype is a lightweight local web UI that validates the core document-calculator
experience before the native Qt/Kirigami shell is built.

It is not the final KDE UI. It exists to test the product shape early:

- source editor on the left;
- result column on the right;
- shared `src/core` evaluation;
- line-level diagnostics;
- synchronized scrolling;
- monospace document typography;
- light/dark system color support.

## Run

```sh
npm run gui
```

Then open:

```text
http://127.0.0.1:15156
```

Optional port override:

```sh
NUMI_KDE_GUI_PORT=15157 npm run gui
```

## Current Scope

Implemented:

- `src/gui/adapter.js`: converts shared core output to a GUI view model;
- `src/gui/server.js`: static prototype server and `/api/evaluate`;
- `gui/index.html`: editor/result-column shell;
- `gui/styles.css`: KDE-like restrained layout, monospace editor/result typography;
- `gui/app.js`: live evaluation and copy-results action;
- `test/gui.test.js`: adapter and server API tests.

## Native KDE Prototype Requirement

The current machine has Qt6 runtime pieces, but the Qt Quick/Kirigami development packages are not
available to CMake:

- missing `Qt6QmlConfig.cmake`;
- missing `Qt6QuickConfig.cmake`;
- missing `KF6KirigamiConfig.cmake`;
- no `qml6` runner in `PATH`.

Before the native KDE prototype can be built, install the Fedora development packages that provide
Qt Quick/QML and Kirigami CMake configs. The exact package names should be verified on the target
system before installation.

## Next GUI Work

- Create `kde/` CMake skeleton once Qt6 Quick/Kirigami dev packages are installed.
- Port the current editor/result-column layout to QML/Kirigami.
- Add semantic syntax highlighting from `parseDocument()` token ranges.
- Add font settings and result column width settings.
- Add screenshot/visual checks for light/dark theme and narrow/wide layouts.
