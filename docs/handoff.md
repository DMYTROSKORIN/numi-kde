# numi-kde Handoff

Last updated: 2026-04-30.
Last completed commit: `fix: Phase 2.1.1 cursor, hover and highlight fixes`.
Last functional GUI commit: current HEAD.

---

## Цель проекта

Сделать KDE/Linux-native клон Numi: текстовый документ слева, результаты справа,
мгновенный пересчёт, синтаксическая подсветка, компактное всегда-доступное окно,
нативное поведение KDE.

**GUI quality — P0.** Всё должно ощущаться как Numi, а не как калькулятор.

---

## Обязательные правила работы

1. Каждое новое поведение — тест перед коммитом.
2. После каждого функционального шага: `npm test`.
3. После любых QML/C++ изменений:
   ```sh
   cmake -S kde -B build/kde
   cmake --build build/kde
   ./build/kde/numi-kde
   ```
4. Обновлять `docs/implementation-plan.md` и этот файл после каждого шага.
5. Коммитить и делать `git push` только после зелёных тестов и чистого запуска.
6. Никакого незакоммиченного прогресса.

---

## Текущее состояние (проверено)

```sh
npm test          # 54/54 pass
cmake -S kde -B build/kde   # pass
cmake --build build/kde     # pass
./build/kde/numi-kde        # запускается без stderr
git status --short          # чисто
```

### Что работает в GUI прямо сейчас

- Frameless тёмное окно, compact Numi-стиль.
- Linux/KDE controls: history (↺) слева, minimize/maximize/close справа.
- Шестерёнка внизу слева — открывает Popup с настройками.
- Настройки: "Всегда поверх окон", "Размер шрифта" (slider), "Ширина результатов" (slider).
- Always-on-top, fontSize и resultWidth сохраняются в Qt Settings.
- Окно resizable со всех сторон, геометрия сохраняется между запусками.
- Редактор слева — активный, editable TextArea с синтаксическим overlay.
- Подсветка: операторы жёлтые (`+−^:=...`), единицы/валюта синие, определённые переменные
  неоновый зелёный `#39ff14`, `%`/`*`/`/` синие.
- Placeholder `/help` (тёмно-серый), пропадает при фокусе.
- Команда `/help` возвращает краткую русскоязычную подсказку в правую колонку.
- Правая колонка: результаты; hover → текст результата становится ярко-жёлтым `#ffd35a`;
  клик копирует в буфер обмена.
- Скролл-синк между редактором и результатами через `Connections`.

### Известные ограничения

| Ограничение | Причина | Когда чинить |
|-------------|---------|--------------|
| `100 USD to AED` — нет результата | JS-движок не знает курсов валют | Phase 2.3 (qalculate) |
| `20% from 100` — нет результата | JS-движок не поддерживает этот синтаксис | Phase 2.3 (qalculate) |
| Always-on-top не работает на нативном Wayland | `Qt.WindowStaysOnTopHint` игнорируется Wayland-композитором; нужен `KWindowSystem::setKeepAbove()` | Phase 2.4 |
| Scroll sync может десинхронизироваться на длинных документах | TextArea не экспонирует `contentY` напрямую | Phase 2.3 |
| History panel — плейсхолдер | Не реализована | Phase 2.5 |
| Node.js worker для вычислений | Для финального пакета нужен embedded runtime или нативный C++ бэкенд | Phase 2.3 |

---

## Важные файлы

```
kde/qml/Main.qml            — окно, флаги, alwaysOnTop, WindowButton/ResizeHandle компоненты
kde/qml/DocumentPage.qml    — layout, settings popup, EditorPane ↔ ResultsPane связь
kde/qml/EditorPane.qml      — TextArea + highlight overlay + FontMetrics, cursorDelegate
kde/qml/ResultsPane.qml     — ListView, hover-цвет, scroll sync Connections
kde/src/documentmodel.h/.cpp — C++ QML-модель, вызывает Node worker
src/gui/adapter.js          — собирает view model: result, highlightedHtml, assignment
src/gui/highlight.js        — classifyHighlightToken, createHighlightedHtml(line, variables)
src/gui/evaluate-document.js — Node worker entry point
src/core/engine.js          — evaluateDocument, evaluateLine, /help спецслучай
src/core/syntax.js          — токенайзер с \p{L} Unicode
test/gui.test.js            — тесты adapter и highlight
test/kde-skeleton.test.js   — структурные тесты QML/CMake
docs/Gemini-qalculate-evolution.md — план миграции на libqalculate
```

---

## Следующие задачи по приоритету

### Phase 2.3 — Миграция на libqalculate (главная архитектурная задача)

**Цель:** заменить JS-движок в нативном GUI на C++ libqalculate.
Это устранит все проблемы с валютой, процентами и расширенной математикой.
Детальный план — в `docs/Gemini-qalculate-evolution.md`.

**Контекст:**
- `libqalculate-devel 5.7.0` уже установлен на Fedora 43.
- Текущий `DocumentModel` (`kde/src/documentmodel.cpp`) запускает Node worker через `QProcess`.
- После миграции Node.js не нужен для работы GUI.

**Шаги:**

1. **Добавить find_package в CMakeLists.txt:**
   ```cmake
   find_package(PkgConfig REQUIRED)
   pkg_check_modules(QALCULATE REQUIRED libqalculate)
   target_include_directories(numi-kde PRIVATE ${QALCULATE_INCLUDE_DIRS})
   target_link_libraries(numi-kde PRIVATE ${QALCULATE_LIBRARIES})
   ```
   Проверить: `cmake -S kde -B build/kde` проходит с найденным libqalculate.

2. **Создать `kde/src/qalcbridge.h` / `qalcbridge.cpp`** — тонкая обёртка:
   ```cpp
   #include <libqalculate/qalculate.h>
   struct LineResult { bool ok; QString result; QString error; };
   class QalcBridge : public QObject {
       Q_OBJECT
   public:
       explicit QalcBridge(QObject *parent = nullptr);
       QList<LineResult> evaluateDocument(const QString &source);
   private:
       Calculator *m_calc;
   };
   ```
   - В конструкторе: `new Calculator(true)`, `m_calc->loadGlobalDefinitions()`.
   - `evaluateDocument` разбивает по `\n`, для каждой строки вызывает
     `m_calc->calculate(line.toStdString(), EvaluationOptions())`, преобразует
     `MathStructure` в строку через `m_calc->print(result, PrintOptions())`.

3. **Переписать `DocumentModel::setSource`** — убрать `QProcess`, вызывать `QalcBridge`:
   - Построчно заполнять `m_lines` из `QalcBridge::evaluateDocument(source)`.
   - Для поля `highlightedHtml` пока оставить вызов к JS highlight через `QProcess`
     (или временно убрать подсветку и добавить вслед за шагом 4).

4. **Перенести логику highlight в C++** (опционально на этом шаге, можно позже):
   - Либо портировать `highlight.js` в Qt QML (вызывать JS прямо из C++ через `QJSEngine`).
   - Либо реализовать упрощённый токенайзер на C++ прямо в `DocumentModel`.

5. **Добавить тесты** в новый файл `test/qalc-bridge.test.js` (или через QTest в C++):
   - `100 + 200 = 300`
   - `100 USD to AED` — должен дать числовой результат (не пустую строку)
   - `20% of 100 = 20`
   - `10 km in miles`
   - Тест что `/help` всё ещё работает (обрабатывается до qalculate)

6. `npm test` → CMake build → runtime check.

7. Обновить документацию, коммит, push.

**Важные предупреждения для qalculate:**
- `Calculator` — синглтон; создавать один раз в `QalcBridge`, не пересоздавать.
- `loadGlobalDefinitions()` медленная — вызывать один раз при старте.
- Потокобезопасность: `m_calc->calculate()` не thread-safe; если DocumentModel
  работает в другом потоке — нужен мьютекс.
- Результат `MathStructure` может быть символьным (не числом) — проверять
  `result.isNumber()` перед форматированием.
- Для валют нужно загрузить курсы: `m_calc->loadExchangeRates()` — делать асинхронно
  при первом обращении к валютному выражению.

---

### Phase 2.4 — Always-on-top на Wayland

**Цель:** чтобы переключатель "Всегда поверх окон" работал и на нативном Wayland KDE.

**Шаги:**

1. Добавить зависимость в CMakeLists.txt:
   ```cmake
   find_package(KF6WindowSystem REQUIRED)
   target_link_libraries(numi-kde PRIVATE KF6::WindowSystem)
   ```

2. В `DocumentModel` или отдельном helper добавить Q_INVOKABLE метод:
   ```cpp
   Q_INVOKABLE void setKeepAbove(bool above);
   ```
   Реализация:
   ```cpp
   #include <KWindowSystem>
   void DocumentModel::setKeepAbove(bool above) {
       if (auto *win = qApp->focusWindow()) {
           KWindowSystem::setKeepAbove(win->winId(), above);
       }
   }
   ```

3. В `Main.qml` заменить прямое переключение флагов на вызов модели:
   ```qml
   onAlwaysOnTopChanged: {
       windowSettings.alwaysOnTop = alwaysOnTop
       documentModel.setKeepAbove(alwaysOnTop)
       root.flags = Qt.Window | Qt.FramelessWindowHint | (alwaysOnTop ? Qt.WindowStaysOnTopHint : 0)
   }
   ```
   (оба вызова: `setKeepAbove` для Wayland, `flags` для X11/XWayland).

4. Тест: структурный — `assert.match(main, /setKeepAbove/)`.

5. CMake build → runtime check → коммит.

---

### Phase 2.5 — Панель истории

**Цель:** кнопка ↺ (historyButton) открывает боковую панель с историей расчётов.

**Шаги (когда дойдёт очередь):**

1. Добавить `kde/qml/HistoryPane.qml` — список сессий/строк с результатами.
2. В `DocumentModel` добавить механизм сохранения истории в `QSettings`.
3. В `Main.qml` добавить drawer или Popup, привязанный к `historyButton.onClicked`.
4. Тесты + документация + коммит.

---

## Архитектурные решения, принятые в проекте

- **JS core (`src/core/`)** — главный источник правды для вычислений. Все фиксируется
  тестами, потом подключается к GUI. UI не содержит математическую логику.
- **`cursorColor` не использовать** — падает с `Controls.TextArea` в Qt 6.10. Вместо него
  `cursorDelegate: Rectangle`.
- **`lineHeight`/`lineHeightMode` не использовать на TextArea** — свойства типа `Text`,
  на TextArea вызывают silent crash.
- **`FontMetrics.height`** — единственный источник `lineH`; overlay и results синхронизируются
  через него.
- **Kirigami не импортировать** в текущем скелете — портит configure/launch на чистой Fedora.
  Вернуть только для settings drawer или навигации.
- **Flags при изменении always-on-top** — нельзя полагаться только на QML binding;
  нужен явный `root.flags = ...` в `onAlwaysOnTopChanged`.
- **Переменные в highlight** — передавать как `Set` в `createHighlightedHtml(line, variables)`;
  проверка переменных идёт раньше entity/operator.
- **`/help` как спецслучай** — обрабатывается в `engine.js` до `parseLine`, возвращает
  `formatted` строку; остаётся актуальным и после qalculate-миграции.

---

## Отказ от кастомного JS-движка

Пользователь принял решение: **заменить JS-математику на libqalculate**. JS-тесты остаются
как compatibility oracle на переходный период. JS core (`src/core/`) не удалять до тех пор,
пока qalculate не покрывает все тестовые случаи. После полной миграции JS core можно
заморозить или убрать из GUI пайплайна.

Концепт приложения сохраняется: минималистичный UX без классического интерфейса
калькулятора. libqalculate — только вычислительный бэкенд, не UI-компонент.
