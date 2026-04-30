# Gemini's Qalculate-Inspired Evolution Plan for numi-kde

## 1. Introduction & Context

This document outlines a strategic evolution plan for the `numi-kde` project, inspired by an analysis of the [Qalculate!](https://qalculate.github.io/) ecosystem, specifically `libqalculate` (the core C++ math engine) and `qalculate-qt` (the Qt-based graphical interface).

Currently, `numi-kde` implements a custom, clean-room JavaScript mathematical evaluator designed to strictly mimic the macOS "Numi" application's natural language parsing and behavior. While this JS core is well-tested and achieves high compatibility with Numi's basic workflow, it inherently lacks the deep mathematical capabilities of a mature, scientific-grade engine.

## 2. Analysis of Qalculate!

### The Engine: `libqalculate`
`libqalculate` is an incredibly powerful, multi-purpose math engine. Its strengths include:
*   **Arbitrary Precision:** Uses GMP and MPFR for exact rational and floating-point arithmetic.
*   **Symbolic Math:** Supports simplification, differentiation, integration, and solving equations/inequalities.
*   **Advanced Number Systems:** Handles complex numbers, infinite values, interval arithmetic, and uncertainty propagation.
*   **Massive Library:** Built-in support for over 400 functions, 400 units, physical constants, and daily-updated currency rates.
*   **Fault-Tolerant Parsing:** Designed to handle natural-ish input and provide verbose diagnostics.

### The UI: `qalculate-qt`
`qalculate-qt` provides a functional but traditional calculator interface. It relies on a hybrid input model (keypad + command line) and uses modal dialogs for advanced management (units, variables, plotting). 

**The Contrast:** While Qalculate! has a vastly superior math engine, its Qt UI is dense and traditional. `numi-kde` aims for a minimalist, text-document workflow (expressions on the left, results on the right) which is aesthetically cleaner and often faster for daily "scratchpad" calculations.

## 3. The Evolution Strategy: Combining the Best of Both Worlds

The ultimate goal for `numi-kde` should be to **marry the sheer mathematical power of `libqalculate` with the elegant, minimalist text-document UX of Numi.**

Here is the proposed phase-by-phase plan:

### Phase 1: Technical Spike - `libqalculate` Integration
Instead of continuing to build out the custom JS evaluator for advanced math, we should evaluate replacing the backend entirely.
1.  **Direct C++ Integration:** Since `numi-kde` is already moving towards a native Qt/C++ shell (`kde/src/documentmodel.cpp`), we can link directly against `libqalculate` via C++.
2.  **Bypass Node.js:** This would eliminate the need to spawn a local Node.js worker process (`src/gui/evaluate-document.js`), dramatically reducing memory overhead, startup time, and packaging complexity for the final Linux Flatpak/AppImage.
3.  **API Mapping:** Map `numi-kde`'s required API (`evaluateLine`, diagnostics, formatting) to `libqalculate`'s parsing and evaluation methods.

### Phase 2: Adapting the UX for Advanced Math
Once the engine is integrated, the UI needs to expose Qalculate's power without losing its minimalist soul.
1.  **Natural Language Wrappers:** Qalculate supports natural language to an extent, but we may need to write a lightweight pre-processor in C++ to translate specific Numi-idioms (like `10% of 200`) into strict `libqalculate` syntax before evaluation.
2.  **Inline Solving:** Support equations natively in the text document. For example, typing `x + 5 = 12` on the left should yield `x = 7` on the right.
3.  **Hover & Tooltips:** Use Qalculate's verbose warnings/errors to provide rich tooltips in the `numi-kde` editor when a calculation is ambiguous.
4.  **Currency & Rates:** Leverage Qalculate's built-in currency updating mechanism instead of writing a custom JS provider.

### Phase 3: Advanced Features in a Minimalist Shell
1.  **Command Palette Integration:** Instead of complex dialogs (like `qalculate-qt`), use the planned Command Palette (`Ctrl+Shift+P`) to search and insert complex units, functions, or physical constants.
2.  **Plotting:** Investigate rendering `libqalculate`'s Gnuplot output as an inline image directly within the right-hand result column of the document.
3.  **Workspaces:** Support saving different `.numi` files with their own isolated `libqalculate` variable/state contexts.

## 4. Conclusion

By pivoting the backend architecture from a custom JS implementation to `libqalculate`, `numi-kde` can instantly gain decades of mathematical engineering, arbitrary precision, and extensive unit/currency support. The core engineering effort can then focus entirely on what makes the app unique: building the best possible QML/Kirigami text-document calculator interface for the KDE Plasma desktop.