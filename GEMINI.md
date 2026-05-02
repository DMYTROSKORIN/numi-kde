# Project Development Plan: numi-kde

This document outlines the roadmap for stabilizing and improving the `numi-kde` project.

## Phase 1: Stabilization & Core Logic (Priority: High)
- [x] **Fix Variable Persistence Bug**: Ensure that variables from deleted lines do not persist in the calculation engine.
- [x] **KWin Rules Safety**: Refactor `kwinrulesrc` modification to be more robust and prevent data loss from concurrent writes.
- [x] **Variable Name Validation**: Improve regex for variable assignments to prevent edge-case collisions with units.

## Phase 2: Performance & Concurrency (Priority: Medium)
- [x] **Async Evaluation**: Move calculation logic to a background thread to keep the UI responsive during heavy computation.
- [x] **QML Model Optimization**: Transition from `Repeater` to a more efficient view (like `ListView`) for rendering results and highlights.
- [x] **Highlighter Optimization**: Cache unit/keyword lookups in `SyntaxHighlighter` to reduce per-token overhead.

## Phase 3: UX & Polish (Priority: Medium)
- [x] **Organization Refactoring**: Rename the organization/app identity from `skorin/numi-kde` to a more formal `numi-kde`.
- [x] **Network Status Feedback**: Add a visual indicator for cryptocurrency rate update status.
- [ ] **HiDPI Stability**: Verify and adjust editor overlay alignment for various scaling factors.

## Phase 4: Testing & CI (Priority: Low)
- [ ] **Extended Test Suite**: Add tests for multi-line variable chains, deletions, and complex date arithmetic.
- [ ] **Mocked Network Tests**: Add tests for crypto conversion that don't depend on live APIs.

---
*Created by Gemini CLI on 2026-05-03.*
