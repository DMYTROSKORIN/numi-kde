import assert from "node:assert/strict";
import test from "node:test";
import {
  createExtensionRegistry,
  evaluate,
  evaluateDocument,
  loadExtensionModules,
  parseDocument,
} from "../src/core/engine.js";
import { documentFixtures, expressionFixtures, formattingFixtures } from "../fixtures/core-compatibility.js";

for (const [name, source, expected] of expressionFixtures) {
  test(`evaluates fixture: ${name}`, () => {
    assert.equal(evaluate(source).formatted, expected);
  });
}

for (const fixture of documentFixtures) {
  test(`evaluates document fixture: ${fixture.name}`, () => {
    const lines = evaluateDocument(fixture.source);
    assert.deepEqual(lines.map((line) => line.formatted), fixture.results);
  });
}

for (const fixture of formattingFixtures) {
  test(`formats fixture: ${fixture.name}`, () => {
    assert.equal(evaluate(fixture.source, { format: fixture.options }).formatted, fixture.expected);
  });
}

test("parseDocument exposes token ranges for future GUI highlighting", () => {
  const [line] = parseDocument("total = 2pi + 1");

  assert.equal(line.ok, true);
  assert.equal(line.assignment.name, "total");
  assert.equal(line.ast.type, "BinaryExpression");
  assert.equal(line.ast.left.type, "BinaryExpression");
  assert.equal(line.ast.left.implicit, true);
  assert.deepEqual(
    line.tokens.map((token) => [token.type, token.raw, token.start, token.end]),
    [
      ["identifier", "total", 0, 5],
      ["assignment", "=", 6, 7],
      ["number", "2", 8, 9],
      ["identifier", "pi", 9, 11],
      ["+", "+", 12, 13],
      ["number", "1", 14, 15],
    ],
  );
});

test("reports parse errors without throwing from document evaluation", () => {
  const [line] = evaluateDocument("unknown + 1");

  assert.equal(line.ok, false);
  assert.match(line.error, /Unknown variable/);
  assert.deepEqual(line.diagnostics, [
    {
      severity: "error",
      message: "Unknown variable: unknown",
      range: { start: 0, end: 7 },
    },
  ]);
});

test("supports extension variables, functions and units", () => {
  const extensions = createExtensionRegistry((numi) => {
    numi.setVariable("answer", 42);
    numi.addFunction("triple", (value) => value * 3);
    numi.addUnit("smoot", {
      dimension: "length",
      ratio: 1.7018,
      aliases: ["smoot", "smoots"],
    });
  });

  assert.equal(evaluate("answer", { extensions }).formatted, "42");
  assert.equal(evaluate("triple(14)", { extensions }).formatted, "42");
  assert.equal(evaluate("2 smoots in m", { extensions }).formatted, "3.4036 m");
});

test("loads extension modules from manifest and source", () => {
  const { diagnostics, registry } = loadExtensionModules([
    {
      manifest: {
        id: "demo-extension",
        apiVersion: 1,
        version: "0.1.0",
        entry: "extension.js",
      },
      source: `
        numi.setVariable("bonus", 7);
        numi.addFunction("quad", value => value * 4);
        numi.addUnit("hand", {
          dimension: "length",
          ratio: 0.1016,
          aliases: ["hand", "hands"]
        });
      `,
    },
  ]);

  assert.deepEqual(diagnostics, []);
  assert.equal(evaluate("bonus", { extensions: registry }).formatted, "7");
  assert.equal(evaluate("quad(3)", { extensions: registry }).formatted, "12");
  assert.equal(evaluate("10 hands in m", { extensions: registry }).formatted, "1.016 m");
});

test("reports extension manifest diagnostics without loading source", () => {
  const { diagnostics, registry } = loadExtensionModules([
    {
      manifest: { apiVersion: 1 },
      source: "numi.setVariable('shouldNotLoad', 1);",
    },
  ]);

  assert.equal(registry.variables.has("shouldNotLoad"), false);
  assert.deepEqual(diagnostics, [
    {
      severity: "error",
      extensionId: "<unknown>",
      message: "Extension manifest id must be a non-empty string",
    },
  ]);
});

test("reports extension runtime diagnostics and keeps earlier modules", () => {
  const { diagnostics, registry } = loadExtensionModules([
    {
      manifest: { id: "ok", apiVersion: 1, entry: "ok.js" },
      source: "numi.setVariable('loaded', 2);",
    },
    {
      manifest: { id: "broken", apiVersion: 1, entry: "broken.js" },
      source: "throw new Error('boom');",
    },
  ]);

  assert.equal(evaluate("loaded", { extensions: registry }).formatted, "2");
  assert.deepEqual(diagnostics, [
    {
      severity: "error",
      extensionId: "broken",
      message: "boom",
    },
  ]);
});
