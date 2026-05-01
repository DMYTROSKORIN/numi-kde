# numi-kde

KDE/Linux-native clone of Numi: a natural language calculator with a text-document interface.

## Goals

- Preserve Numi's text-document workflow: expressions on the left, results on the right.
- **100% C++/Qt Backend**: Zero Node.js dependency during runtime for maximum speed and compatibility.
- **libqalculate Engine**: Powerful calculation core with support for currencies, units, and complex math.
- **KDE Integration**: Native Wayland/X11 support, persistent window geometry, and Always-on-Top mode.
- **History & Sessions**: Persistent session management with quick recall via side panel.
- **Tab Completion**: Terminal-like autocompletion for units, functions, and variables.

## Features

- **Math**: `2 + 2 * 3^2`, `sqrt(256)`, `sin(pi/2)`
- **Units**: `10 meters in feet`, `50kg to lbs`, `200 m2 to sq`
- **Currency**: `100 USD to EUR`, `50 EUR in JPY`
- **Dates**: `today + 2 weeks`, `now - 256 days`
- **Variables**: `A = 800 - 200`, `B := A * 2`
- **UI**: English localization, adjustable font size and result column width, precise decimal control.

## Build and Run

### Prerequisites

- Qt 6.6+
- KDE Frameworks 6 (KF6WindowSystem)
- libqalculate
- CMake

### Commands

```sh
# Build
cmake -S kde -B build/kde
cmake --build build/kde

# Run
./build/kde/numi-kde

# Test (legacy JS oracle + native skeleton tests)
npm test
```

## Documentation

- `docs/implementation-plan.md`: Detailed roadmap and completion status.
- `docs/handoff.md`: Project status and architectural overview.
- `docs/Gemini-qalculate-evolution.md`: Strategy for libqalculate integration.
