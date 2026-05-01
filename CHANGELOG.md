# Changelog

## 0.1.0 - 2026-05-01

Release candidate for the native KDE runtime.

- Reworked `/help` into a readable in-app guide with examples for math, variables, units, currency, crypto, dates, percentages and controls.
- Added adaptive result-column sizing for long results.
- Improved result separator dragging by using page-space pointer coordinates and clamping the minimum width to the rendered result content.
- Fixed `Total` so converted unit/currency/crypto rows contribute their numeric result values.
- Added explicit date-span support for inputs such as `today - 26.08.1983`.
- Added native tests for date spans, converted-result totals and numeric conversion metadata.
- Removed the legacy JavaScript/web prototype so the repository contains only the native KDE project.
- Removed stale includes from `DocumentModel`.

Verification:

```sh
cmake --build build/kde --target numi-kde numi-kde-tests
ctest --test-dir build/kde --output-on-failure
```
