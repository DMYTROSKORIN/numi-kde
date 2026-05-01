# Changelog

## 0.1.2 - 2026-05-01

- Matched the result separator default color to the app divider color used by the Total row and settings controls.
- Added operator highlighting inside inline `/help` examples without changing the help layout.
- Changed Total semantics so totals are shown only when every numeric row has the same total key; mixed units/currencies hide Total.
- Removed agent-only documents from the repository.
- Refreshed documentation to describe compatible totals and the cleaned native-only project structure.

## 0.1.1 - 2026-05-01

- Changed `/help` from a panel/card overlay to inline editor help text.
- Made the result separator visibly present while keeping the lag-free drag behavior.
- Changed Total to appear only for compatible numeric rows; mixed units/currencies such as `500 AED to USD` plus `1 BTC to UAH` no longer show a total.
- Added `today` operator highlighting.
- Added date arithmetic for inputs such as `26.08.1983 + 42 years`.
- Added regression tests for the reported help, total, highlighting and date arithmetic cases.

## 0.1.0 - 2026-05-01

Release candidate for the native KDE runtime.

- Reworked `/help` into a readable in-app guide with examples for math, variables, units, currency, crypto, dates, percentages and controls.
- Added adaptive result-column sizing for long results.
- Improved result separator dragging by using page-space pointer coordinates and clamping the minimum width to the rendered result content.
- Added typed Total support for compatible converted unit/currency/crypto rows.
- Added explicit date-span support for inputs such as `today - 26.08.1983`.
- Added native tests for date spans, converted-result totals and numeric conversion metadata.
- Removed non-native files so the repository contains only the native KDE project.
- Removed stale includes from `DocumentModel`.

Verification:

```sh
cmake --build build/kde --target numi-kde numi-kde-tests
ctest --test-dir build/kde --output-on-failure
```
