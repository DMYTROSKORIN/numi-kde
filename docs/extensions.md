# Extension Policy

`numi-kde` targets the public Numi-shaped JavaScript API:

- `numi.setVariable(name, value)`
- `numi.addFunction(name, fn)`
- `numi.addUnit(id, definition)`

This document describes the current loading policy. It is intentionally conservative and will be
expanded before extensions are loaded from user directories.

## Manifest

Each extension module must provide a manifest:

```json
{
  "id": "demo-extension",
  "apiVersion": 1,
  "version": "0.1.0",
  "entry": "extension.js"
}
```

Required fields:

- `id`: non-empty string used in diagnostics and future settings UI;
- `apiVersion`: must currently be `1`.

Optional fields:

- `version`: extension version for display and future compatibility checks;
- `entry`: source filename used in diagnostics.

Invalid manifests are reported as diagnostics and their source is not executed.

## Runtime

The current implementation loads extension source into a restricted Node `vm` context containing:

- `numi`
- `Math`
- `Number`
- `String`

It does not expose `process`, `require`, filesystem APIs, network APIs, timers, or module loading.
Execution has a short timeout.

Important: Node `vm` is a development-time isolation layer, not a final security sandbox. Before
loading arbitrary user extensions from disk, this policy must be revisited. A production KDE build
should prefer one of:

- a dedicated extension worker process with a narrow IPC protocol;
- a WebAssembly or QuickJS-style runtime with explicit host bindings;
- disabled-by-default extensions with clear trust UI.

## Diagnostics

Extension loading never throws through the public core API. It returns diagnostics:

```js
{
  severity: "error",
  extensionId: "demo-extension",
  message: "boom"
}
```

GUI should display these diagnostics in the Extensions settings page and status surface.

## Current Supported API

### Variables

```js
numi.setVariable("answer", 42)
```

Variables become available to the evaluator as normal identifiers.

### Functions

```js
numi.addFunction("triple", value => value * 3)
```

Functions receive numeric arguments and must return a numeric value.

### Units

```js
numi.addUnit("smoot", {
  dimension: "length",
  ratio: 1.7018,
  aliases: ["smoot", "smoots"]
})
```

Only linear units are supported in the current skeleton. `ratio` is the multiplier to the
dimension base unit.

## Next Work

- Define extension directory layout.
- Add enable/disable state per extension.
- Add extension load order.
- Add structured extension logs for the KDE settings page.
- Replace or harden the runtime before loading arbitrary user files by default.
