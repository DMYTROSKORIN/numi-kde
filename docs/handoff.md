# numi-kde Handoff

Last updated: 2026-05-01.
Last completed commit: `feat: functional fixes and settings window`.

---

## Цель проекта

Сделать KDE/Linux-native клон Numi: текстовый интерфейс для вычислений,
минималистичный UX, полная интеграция с плазмой.
Математическое ядро перенесено на **libqalculate** для 100% точности и поддержки
валют/единиц измерения.

---

### Что работает в GUI прямо сейчас

- Frameless тёмное окно, compact Numi-стиль.
- Linux/KDE controls: history (↺) слева, minimize/maximize/close справа.
- Шестерёнка внизу слева — открывает отдельное нативное окно настроек.
- Настройки: "Always on top", "Font size", "Result width", "Decimal places".
- Все вычисления и подсветка синтаксиса на C++ (libqalculate).
- Поддержка переменных (`A = 100`), юнитов (`200 m2 to sq`), дат (`now - 2 days`).
- Always-on-top работает на Wayland и X11 через KWindowSystem.
- Окно resizable, геометрия сохраняется.
- Скролл-синк между редакторами и результатами.

### Известные ограничения

| Ограничение | Причина | Когда чинить |
|-------------|---------|--------------|
| Scroll sync может десинхронизироваться на длинных документах | TextArea не экспонирует `contentY` напрямую | Phase 2.3 |
| History panel | Полностью реализована: сохранение сессий в QSettings, Drawer сбоку | Done |
| Tab completion — нет | Не реализована | Phase 2.7 |
| Зависимость от Node.js в GUI | Полностью удалена. | Done |

---

## Важные файлы

```
kde/qml/Main.qml            — главное окно + Drawer (история)
kde/qml/SettingsWindow.qml  — окно настроек
kde/qml/HistoryPane.qml     — список сохранённых сессий
kde/qml/DocumentPage.qml    — основная рабочая область
kde/qml/EditorPane.qml      — редактор
kde/qml/ResultsPane.qml     — результаты
kde/src/documentmodel.h/.cpp — модель данных (вкл. логику истории)
kde/src/qalcbridge.h/.cpp    — мост к libqalculate
kde/src/syntaxhighlighter.h/.cpp — подсветка синтаксиса
```

---

## Следующие задачи по приоритету

### Phase 2.7 — Автодополнение (Tab)

**Цель:** подсказки для команд, юнитов и переменных.


---

## Архитектурные решения

- **100% C++ для GUI** — Node.js удалён из runtime зависимостей для скорости и совместимости.
- **libqalculate** — единственный движок расчётов.
- **Custom SyntaxHighlighter** — нативный C++ класс для раскраски TextArea.
- **KWindowSystem** — используется для управления состоянием окна на разных дисплейных серверах.
