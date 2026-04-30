# Architecture

## Product Shape

Numi's durable workflow is a document calculator:

- users type plain text lines;
- each meaningful line receives a result;
- variables and tokens can refer to earlier lines;
- documents are saved as plain `.numi` text files.

`numi-kde` keeps that workflow independent from the desktop shell. The core must work in tests,
CLI, a local HTTP endpoint, and the KDE UI.

## Layers

1. `core`: parser, evaluator, units, variables, formatting, plugin registry.
2. `cli`: command-line entry point compatible with one-shot Numi CLI usage.
3. `server`: local `localhost:15055?q=...` compatibility endpoint.
4. `kde`: Qt/Kirigami interface, global shortcut, tray behavior, clipboard workflows.
5. `krunner`: KRunner plugin backed by the same core/server.

## Compatibility Strategy

Public Numi source code is not available in the cloned repository. Compatibility should therefore
be driven by fixtures collected from:

- public documentation and wiki examples;
- the existing Alfred workflow contract;
- public JavaScript extension examples;
- black-box comparisons against official Numi builds where available.

Each supported behavior should have a fixture before being considered stable.

## Implementation Notes

The first version is dependency-free modern JavaScript so development can start on the current
machine without installing Rust or Qt. The core API should stay narrow enough to allow a later
Rust or C++ implementation behind the same tests if needed.

Qt/Kirigami work should begin after the core has enough coverage for arithmetic, variables,
units, percentages, dates, and formatting. That avoids baking parser behavior into UI code.
