# План реализации numi-kde

## Цель

`numi-kde` должен стать KDE-native клиентом, максимально близким к Numi по рабочему
процессу, внешнему виду и поведению:

- текстовый документ слева, результаты справа;
- мгновенный пересчет при наборе;
- сохранение `.numi` файлов как обычного UTF-8 текста;
- совместимость с CLI и локальным HTTP endpoint `localhost:15055?q=...`;
- поддержка пользовательских единиц, функций и переменных;
- интеграция с KDE: Kirigami UI, KRunner, global shortcut, tray, clipboard, file dialogs,
  system theme, packaging.

Главный принцип: сначала фиксируем поведение вычислителя тестами, затем подключаем это же
поведение к GUI, CLI, server и KRunner. UI не должен содержать вычислительную логику.

## Текущее состояние

Уже есть минимальный scaffold:

- `src/core/engine.js`: базовый evaluator;
- `src/cli.js`: one-shot CLI;
- `src/server.js`: совместимый HTTP endpoint;
- `test/core.test.js`: первые проверки поведения;
- `docs/architecture.md`: базовые архитектурные границы.

Это пока не KDE-приложение. Текущая версия пригодна для проверки ядра, CLI и endpoint.

## P0 Workflow Rules

Эти правила обязательны для каждого следующего шага:

- каждая новая функция или поведение добавляется вместе с fixture/test;
- после каждого реализованного блока запускается `npm test`;
- после каждого блока обновляется этот план с текущим статусом и следующим шагом;
- если меняется пользовательское поведение, обновляется README или профильная документация;
- после зеленых тестов изменения коммитятся и пушатся в GitHub;
- незакоммиченный прогресс не считается завершенным шагом.

## Журнал прогресса

### 2026-04-30

Выполнено:

- добавлен подробный roadmap в `docs/implementation-plan.md`;
- зафиксированы P0 workflow rules: tests, plan/docs update, commit and GitHub push after
  each completed block;
- добавлены fixture-driven compatibility tests в `fixtures/core-compatibility.js`;
- formatter вынесен в `src/core/formatter.js`;
- unit registry и conversions вынесены в `src/core/units.js`;
- tokenizer/parser вынесены в `src/core/syntax.js`;
- parser теперь возвращает AST и token ranges для будущей GUI-подсветки;
- diagnostics вынесены в `src/core/errors.js` и включают source ranges;
- `parseDocument()` возвращает tokens, AST и assignment metadata;
- добавлен deterministic ISO date слой в `src/core/dates.js`;
- formatter получил options для precision, fixed fraction digits, grouping separators
  and scientific notation threshold;
- добавлен extension registry skeleton в `src/core/extensions.js` для `numi.setVariable`,
  `numi.addFunction`, `numi.addUnit`;
- добавлена extension loading policy в `docs/extensions.md`;
- extension loader теперь валидирует manifest, запускает source в ограниченном `vm` context
  и возвращает diagnostics вместо падения evaluator;
- добавлен первый runnable GUI prototype в `gui/` и `src/gui/`;
- добавлена документация GUI prototype в `docs/gui-prototype.md`;
- определены Fedora dev dependencies для runnable native prototype: `qt6-qtbase-devel`,
  `qt6-qtdeclarative-devel`;
- добавлен native KDE/Qt Quick skeleton в `kde/`;
- добавлена документация native KDE skeleton в `docs/kde-native.md`;
- native KDE shell собран локально через CMake после установки Qt dev packages;
- native shell визуально приближен к Numi reference: compact dark frameless window,
  traffic-light controls, yellow/blue syntax accents, green results;
- direct Kirigami imports removed from the first runnable native prototype to keep Fedora
  configure/build/runtime output clean; KDE desktop style is still requested through Qt Quick
  Controls;
- приняты GUI requirements:
  - приложение Linux/KDE-first, без macOS traffic-light decoration;
  - окно должно быть resizable и editable;
  - bottom-left control is settings gear;
  - bottom-right arrow is removed until needed;
  - top-left control is history;
  - top-right controls: close, minimize, maximize/fullscreen;
  - window stays always on top by default, later configurable;
- implemented GUI requirements in native prototype; app builds and starts without runtime stderr;
- реализованы первые Phase 1 gaps: implicit multiplication, `:=`, inline comments,
  thousands separators, decimal comma, расширенные units, temperature conversions,
  Numi-like percentage cases, ISO date arithmetic, extension-defined variables/functions/units;
- проверка: `npm test` проходит, 50/50; `cmake -S kde -B build/kde` and
  `cmake --build build/kde` pass cleanly; native GUI starts without stderr output.

Следующий шаг:

- подключить live evaluation backend к native shell.

## Опорные технологии

### Runtime ядра

Краткосрочно:

- сохраняем JS core как быстрый clean-room слой для фиксации поведения;
- все новые правила языка сначала покрываем fixtures и `node --test`;
- CLI и server продолжают использовать JS core напрямую.

Среднесрочно:

- KDE shell строим на Qt 6, KDE Frameworks 6 и Kirigami;
- во время разработки GUI можно подключать core через отдельный local worker process;
- backend API должен быть таким же, как у core: `evaluateLine`, `evaluateDocument`,
  diagnostics, tokens, formatted results.

Финально:

- выбрать один production-вариант:
  - портировать core в C++/Rust и оставить JS tests как compatibility oracle;
  - либо встроить JS runtime/worker в пакет, если это окажется надежнее и проще для поддержки;
- final package не должен требовать ручной установки Node.js пользователем.

### KDE UI

Основной UI: Qt Quick/QML + Kirigami, backend на C++.

Причины:

- Kirigami рассчитан на KDE/Plasma и адаптивные интерфейсы;
- Qt Quick Controls дают хорошую базу для textarea, actions, dialogs и settings;
- KDE Frameworks дают KConfig, KXmlGui/KAction, KIO, KNotifications, KRunner и KGlobalAccel.

Для редактора нужно отдельно проверить два варианта:

- QML `TextArea`/`TextEdit` с C++ backend для highlighter и result overlay;
- QWidget/QPlainTextEdit wrapper, если QML редактор не даст достаточно точного контроля над
  caret, gutter, line layout, IME, bidi text и large-document performance.

Решение принимается после технического spike с прототипом редактора на 500-1000 строк.

## Definition of Done

Проект можно считать готовым к первому public release, когда выполнено все:

- поведение arithmetic, units, variables, dates, currencies, percentages и formatting покрыто
  compatibility fixtures;
- GUI повторяет Numi workflow: писать текстом, видеть результат справа, сохранять документ,
  копировать результат, быстро открывать окно;
- KDE integration работает на Wayland и X11;
- KRunner plugin возвращает те же результаты, что CLI/server;
- `.numi` файлы открываются из file manager и из CLI;
- настройки сохраняются через KDE-native config;
- есть Flatpak или другой reproducible package, который запускается на актуальных KDE Linux;
- CI прогоняет unit, integration, formatting, package smoke tests;
- есть ручной visual checklist со screenshots для light/dark theme, HiDPI, fractional scaling.

## Фаза 1. Compatibility Core

Цель: сделать вычислитель достаточно близким к Numi, чтобы UI уже подключался к стабильному
контракту.

### 1.1. Язык выражений

Реализовать и покрыть fixtures:

- arithmetic precedence: `+`, `-`, `*`, `/`, `^`, parentheses;
- unary plus/minus;
- implicit multiplication: `2(3+4)`, `2pi`, `10 kg`;
- decimal separator handling: `1.5`, later optional locale-aware comma;
- thousands separators;
- comments and empty lines;
- assignment: `x = 10`, `total := ...` if compatible;
- references to previous lines or named variables;
- result tokens if Numi supports them in target compatibility set;
- constants: `pi`, `e`, common aliases;
- math functions: `sqrt`, `sin`, `cos`, `tan`, `log`, `ln`, `round`, `floor`, `ceil`, `abs`,
  `min`, `max`, `sum`, `avg`;
- error recovery per line: one invalid line must not break the whole document.

### 1.2. Units

Implement groups with canonical base units:

- length: mm, cm, m, km, inch, foot, yard, mile;
- area and volume;
- mass: mg, g, kg, ton, oz, lb;
- time: ms, s, min, hour, day, week, month/year as calendar-aware only where needed;
- data sizes: bit, byte, KB/KiB, MB/MiB, GB/GiB;
- temperature: C, F, K with offset conversions;
- speed, pressure, energy, power if fixtures confirm Numi behavior;
- unit aliases, plural forms and short symbols.

Each unit conversion must store:

- numeric value;
- canonical unit;
- display unit;
- precision policy;
- source range for syntax highlighting.

### 1.3. Percentages

Cover natural Numi-like cases:

- `10% of 200`;
- `200 + 10%`;
- `200 - 10%`;
- `10 as % of 200`;
- percentage change: `from 100 to 120`;
- chained percentage expressions.

### 1.4. Dates and times

Implement only with fixtures:

- date parsing for ISO first: `2026-04-30`;
- date arithmetic: `today + 2 weeks`;
- differences: `2026-05-15 - 2026-04-30`;
- named days only if unambiguous;
- timezone policy documented;
- locale-sensitive rendering separated from calculation.

### 1.5. Currencies

Currency support must be explicit because rates change.

Plan:

- static currency syntax first: `10 USD in EUR` returns controlled fixture result only in tests;
- pluggable rate provider interface;
- cache with timestamp and offline behavior;
- UI must show rate date/source when live rates are used;
- tests must mock provider, never depend on live network.

### 1.6. Formatting

Create a formatting module independent from parser:

- precision rules;
- trailing zero policy;
- grouping separators;
- scientific notation threshold;
- unit spacing;
- approximate marker if needed;
- locale-aware display as a UI option, while `.numi` source stays plain text.

### 1.7. Extension API

Target Numi-shaped JS API:

- `numi.addUnit`;
- `numi.addFunction`;
- `numi.setVariable`.

Implementation steps:

- define extension manifest and loading locations;
- sandbox extension execution;
- version API;
- surface extension diagnostics in GUI;
- fixtures for extension examples.

## Фаза 2. Core API Contract

GUI, CLI, server and KRunner must use one stable API.

### Required API

`evaluateLine(source, context, options)`:

- returns result, formatted text, diagnostics, tokens, dependencies.

`evaluateDocument(source, options)`:

- returns one item per line;
- preserves line numbers;
- exposes changed ranges for incremental updates later.

`parseDocument(source, options)`:

- returns AST/tokens without executing conversions that need providers.

`formatResult(value, options)`:

- deterministic formatting for tests and UI.

### Diagnostics

Diagnostics must include:

- severity: info, warning, error;
- message id;
- human message;
- line/column range;
- optional quick fix;
- optional docs hint.

GUI uses the same diagnostics for underline, gutter markers and tooltip text.

## Фаза 3. KDE Application Skeleton

Цель: получить настоящее окно KDE-приложения с подключенным core API.

### Structure

Proposed tree:

```text
kde/
  CMakeLists.txt
  src/
    main.cpp
    appcontroller.cpp
    documentmodel.cpp
    evaluationbackend.cpp
    settings.cpp
    syntaxhighlighter.cpp
  qml/
    Main.qml
    DocumentPage.qml
    EditorPane.qml
    ResultsPane.qml
    SettingsPage.qml
    CommandPalette.qml
  resources/
    org.skorin.numi-kde.desktop
    org.skorin.numi-kde.metainfo.xml
    icons/
```

### App shell

Use:

- `Kirigami.ApplicationWindow`;
- native title/menu behavior where KDE expects it;
- actions with standard shortcuts;
- KDE icon names from the FreeDesktop icon theme;
- KConfig-backed settings;
- KIO/XDG portals for file dialogs in sandboxed packages.

Core actions:

- New document;
- Open;
- Save;
- Save As;
- Duplicate line;
- Copy result;
- Copy all results;
- Clear document;
- Toggle comments;
- Find;
- Command palette;
- Preferences;
- About.

## Фаза 4. GUI Design Specification

GUI is the most important product surface. It must feel like a focused document editor, not a
generic calculator and not a marketing page.

### 4.1. Main Window Layout

Desktop default:

```text
+-------------------------------------------------------------+
| Header / toolbar                                            |
+-------------------------------+-----------------------------+
| expression editor             | result column               |
|                               |                             |
|  x = 10                       | 10                          |
|  x * 2                        | 20                          |
|  20 inches in cm              | 50.8 cm                     |
|                               |                             |
+-------------------------------+-----------------------------+
| status: saved, diagnostics, rate timestamp, mode            |
+-------------------------------------------------------------+
```

Layout rules:

- editor and results scroll together line-by-line;
- result column is visually quieter than source column;
- result column width is resizable and remembered;
- long results truncate with fade/ellipsis, full result available by selection/copy/tooltip;
- no cards around the editor;
- no decorative gradients or oversized hero sections;
- all chrome follows KDE/Breeze theme.

Window modes:

- full document mode;
- compact quick-calc mode opened by global shortcut;
- optional always-on-top scratchpad mode.

### 4.2. Editor Behavior

Must support:

- multi-line plain text editing;
- standard shortcuts: copy, paste, cut, undo, redo, select all, save, open, find;
- mouse selection across lines;
- keyboard-only workflow;
- IME input;
- HiDPI and fractional scaling;
- line wrapping optional, default off for desktop;
- synchronized result alignment with wrapped lines;
- configurable tab behavior;
- persistent cursor and scroll position per document if practical.

Editor should feel close to a code editor, but simpler:

- no project tree;
- no tabs in v1 unless multiple documents are explicitly added;
- no rich text source;
- source file remains plain UTF-8.

### 4.3. Typography

Defaults:

- source font: system monospace from KDE font settings;
- result font: same family as source, medium weight only if it improves scanning;
- base size: follow system fixed-width font size;
- result size: equal to source by default;
- line height: 1.35-1.45 of font size;
- minimum editor font size: 9 pt;
- maximum normal UI setting: 24 pt;
- separate UI text uses KDE application font.

Settings:

- source font family;
- source font size;
- result font size mode: same as source, smaller, larger, custom;
- line spacing;
- result column width;
- optional bold results;
- optional dim inactive/comment lines.

Accessibility:

- respect system font scaling;
- no negative letter spacing;
- never encode meaning by color only;
- preserve contrast in light and dark themes;
- support keyboard navigation for every command.

### 4.4. Syntax Highlighting

Highlighting is semantic, not just regex, once parser tokens are available.

Token classes:

- numbers;
- operators;
- parentheses/brackets;
- variables;
- assignment operator;
- functions;
- constants;
- units;
- dates;
- strings if extensions need them;
- comments;
- errors;
- warnings;
- external/live values such as currencies.

Visual policy:

- use KDE theme colors where possible;
- custom syntax colors live only inside editor content;
- provide light and dark palettes;
- allow "follow theme" default;
- allow custom colors later, but not required for v1;
- errors get underline/wave plus gutter marker;
- current line subtly highlighted;
- matching parentheses highlighted near cursor;
- selected text remains readable with system selection color.

Implementation:

- parser emits token ranges;
- highlighter consumes token ranges and diagnostics;
- if QML editor is used, bridge through `QQuickTextDocument`/`QTextDocument`;
- if QWidget editor is used, use `QSyntaxHighlighter` directly;
- highlighter must rehighlight changed blocks only when possible.

### 4.5. Results Column

Result rendering rules:

- one visual result row per source line;
- empty/comment-only lines show no result;
- invalid lines show compact diagnostic marker instead of noisy text;
- result text right-aligned by default;
- units kept together with numeric value;
- copy action copies raw formatted result without decoration;
- hover/focus shows full result and diagnostics;
- clicking a result selects/copies according to setting.

Result states:

- normal;
- approximate;
- stale because live provider failed;
- warning;
- error;
- loading for async providers;
- copied feedback.

### 4.6. Status Bar

Keep status compact:

- document modified/saved state;
- current line/column;
- first diagnostic summary;
- currency rate timestamp if applicable;
- extension errors count;
- background calculation state.

No persistent instructional text in the main UI.

### 4.7. Preferences

Use Kirigami settings pages/FormLayout.

Sections:

- General:
  - launch at login;
  - restore last document;
  - quick-calc window behavior;
  - tray icon.
- Editor:
  - font family;
  - font size;
  - result font size;
  - line height;
  - wrapping;
  - result column width;
  - show line numbers;
  - highlight current line;
  - matching parentheses.
- Formatting:
  - precision;
  - grouping separators;
  - decimal separator policy;
  - locale-aware output;
  - scientific notation threshold.
- Units and currencies:
  - default currency;
  - rate provider;
  - refresh interval;
  - offline cache behavior.
- Extensions:
  - extension directories;
  - enable/disable extensions;
  - diagnostics/log view.
- Shortcuts:
  - global shortcut;
  - copy result;
  - quick open.

### 4.8. Command Palette

Keyboard-first command palette:

- open with `Ctrl+Shift+P` or KDE-appropriate alternative;
- fuzzy search actions;
- show shortcuts;
- run app commands;
- later: insert unit/function snippets.

### 4.9. File Handling

Support:

- open/save `.numi`;
- autosave recovery file;
- recent files;
- file association;
- drag-and-drop `.numi`;
- warn on external file changes;
- preserve final newline policy.

### 4.10. Visual Regression Checklist

Capture screenshots for:

- light Breeze;
- dark Breeze;
- custom accent color;
- 100%, 125%, 150%, 200% scaling;
- narrow quick-calc window;
- wide document window;
- long lines;
- long results;
- errors and warnings;
- selected text;
- focus rings;
- RTL text smoke test.

## Фаза 5. KRunner Integration

KRunner plugin should call the same evaluation backend.

Behavior:

- typing expression in KRunner shows result;
- Enter copies result or inserts it according to KDE convention;
- invalid expressions fail quietly unless there is a useful hint;
- plugin can be disabled independently;
- plugin has no separate parser behavior.

Testing:

- unit tests against backend;
- manual KRunner smoke test on KDE Wayland and X11.

## Фаза 6. Global Shortcut, Tray, Clipboard

Features:

- global shortcut to show/hide quick-calc window;
- tray icon optional;
- copy current line result;
- copy selected result;
- paste result back to active app if user enables it;
- clipboard history friendly behavior.

Wayland note:

- avoid hacks for focus/clipboard;
- use KDE/portal-supported APIs where possible.

## Фаза 7. Packaging

Targets:

- development: local CMake build for KDE shell plus npm tests for JS core;
- release: Flatpak first because it handles runtime dependencies consistently;
- later: AppImage or distro packages if needed.

Package contents:

- desktop file;
- AppStream metadata;
- icons;
- MIME registration for `.numi`;
- KRunner plugin metadata;
- translations;
- license and notices;
- extension directory policy.

Smoke tests:

- app launches from desktop file;
- opens a `.numi` file;
- saves a file;
- KRunner plugin loads;
- CLI/server still work if shipped;
- package works without manually installed Node.js.

## Фаза 8. Testing Strategy

### Unit tests

- parser;
- evaluator;
- units;
- formatter;
- diagnostics;
- extension API;
- settings serialization.

### Compatibility fixtures

Each fixture stores:

- input source;
- expected per-line result;
- expected diagnostics;
- optional Numi observed output;
- notes about source and compatibility confidence.

### Integration tests

- CLI one-shot;
- server endpoint;
- KDE backend document model;
- file open/save;
- KRunner backend adapter.

### GUI tests

Use whichever is practical for KDE stack:

- Qt Test for backend and models;
- QML tests for component states;
- screenshot checks for visual layout;
- manual checklist before releases.

Critical GUI cases:

- no overlap between source and result;
- result rows align with source rows;
- font changes do not break layout;
- long text does not resize toolbar/buttons incorrectly;
- text remains readable in dark/light themes.

## Фаза 9. Milestones

### M0. Documented plan

- This file exists.
- README points to it.

### M1. Core compatibility baseline

- Status: in progress.
- Done: arithmetic, variables, first units, first formatting and percentages are fixture-covered.
- Done: deterministic ISO date literals, date +/- days/weeks, and ISO date differences are fixture-covered.
- Done: formatting options for precision, trailing zeros, grouping and scientific notation are fixture-covered.
- Done: extension-defined variables, functions and linear units are tested through the public core API.
- Done: extension manifest validation, source loading and runtime diagnostics are tested.
- Done: first runnable GUI prototype uses shared core through `src/gui/adapter.js`.
- Done: native KDE skeleton exists and is covered by structure tests.
- Done: native KDE shell builds locally and matches the first Numi visual reference pass.
- Done: CLI/server remain stable against the shared core.
- Done: diagnostics include source ranges for parser/evaluator errors.
- Done: parser/evaluator/formatter/units are split into separate modules.
- Next: wire live evaluation backend into the native KDE shell.

### M2. KDE prototype

- Qt/Kirigami app opens;
- source editor accepts text;
- result column updates from core;
- settings window has first editor font controls.

### M3. Editor quality pass

- syntax highlighting;
- diagnostics underline/gutter;
- matching parentheses;
- synchronized scrolling;
- configurable font size and result column.

### M4. File workflow

- open/save `.numi`;
- recent files;
- modified indicator;
- autosave recovery.

### M5. KDE integration

- global shortcut;
- tray option;
- KRunner plugin;
- file association.

### M6. Extensions and currencies

- extension loader;
- mocked and live currency provider;
- settings and diagnostics.

### M7. Packaging beta

- Flatpak package;
- AppStream metadata;
- visual regression checklist;
- install/run smoke tests.

### M8. Release candidate

- all compatibility fixtures green;
- GUI checklist completed;
- known gaps documented;
- package works on clean KDE system.

## Immediate Next Steps

1. Wire native shell to live evaluation backend.
2. Replace static sample rows with live evaluated document rows.
3. Add semantic syntax highlighting using `parseDocument()` token classes.
4. Add a first `docs/gui-spec.md` or implement the KDE skeleton directly from section 4.
5. Create a small Qt/Kirigami prototype that renders editor + result column.

## Technical References

- KDE Kirigami application structure and QML/Kirigami direction:
  https://develop.kde.org/docs/getting-started/kirigami/
- KDE/Kirigami color guidance: follow system colors and avoid hardcoded chrome colors:
  https://develop.kde.org/docs/getting-started/kirigami/style-colors/
- Kirigami actions, shortcuts and icon-based actions:
  https://develop.kde.org/docs/getting-started/kirigami/components-actions/
- Kirigami FormLayout for settings pages:
  https://develop.kde.org/docs/getting-started/kirigami/components-formlayouts/
- KDE Flatpak packaging and KDE runtime:
  https://develop.kde.org/docs/packaging/flatpak/packaging/
- Qt `TextArea` as a QML multiline editor baseline:
  https://doc.qt.io/qt-6/qml-qtquick-controls-textarea.html
- Qt `QSyntaxHighlighter` for semantic highlighting over `QTextDocument`:
  https://doc.qt.io/qt-6/qsyntaxhighlighter.html
