import assert from "node:assert/strict";
import test from "node:test";
import { createExtensionRegistry, evaluate, evaluateDocument, parseDocument } from "../src/core/engine.js";
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
