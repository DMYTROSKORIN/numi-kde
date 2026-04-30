# numi-kde

Private clean-room implementation of a Numi-compatible natural language calculator for Linux/KDE.

The project starts with a dependency-free calculation core and CLI. KDE UI, KRunner integration,
plugin compatibility, and packaging will be layered on top of the same core.

## Goals

- Preserve Numi's text-document workflow: expressions on the left, results on the right.
- Support `.numi` files as UTF-8 plain text.
- Provide a CLI compatible with the common `numi-cli "20 inches in cm"` workflow.
- Expose a local query endpoint compatible with the Alfred workflow contract:
  `http://localhost:15055?q=...`.
- Support JavaScript extensions using the public `numi.addUnit`, `numi.addFunction`,
  and `numi.setVariable` API shape.
- Build a KDE-native shell with Qt/Kirigami once the core behavior is stable.

## Current State

This is the first scaffold:

- `src/core/engine.js`: arithmetic evaluator, variables, multi-line context, basic units.
- `src/cli.js`: one-shot CLI.
- `src/server.js`: local `q` endpoint compatible with the Alfred workflow shape.
- `test/core.test.js`: executable compatibility checks.
- `docs/architecture.md`: implementation plan and boundaries.
- `docs/implementation-plan.md`: detailed roadmap toward the KDE-native application.
- `docs/extensions.md`: current JavaScript extension loading policy.
- `docs/gui-prototype.md`: first runnable GUI prototype and native KDE requirements.
- `docs/kde-native.md`: native Qt/KF6/Kirigami skeleton build notes.

## Usage

```sh
npm test
node ./src/cli.js "20 inches in cm"
node ./src/cli.js "x = 10\nx * 2"
npm run server
npm run gui
```
