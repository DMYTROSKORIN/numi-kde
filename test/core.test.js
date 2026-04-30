import assert from "node:assert/strict";
import test from "node:test";
import { evaluate, evaluateDocument } from "../src/core/engine.js";

test("evaluates arithmetic with precedence", () => {
  assert.equal(evaluate("2 + 3 * 4").formatted, "14");
  assert.equal(evaluate("(2 + 3) * 4").formatted, "20");
  assert.equal(evaluate("2^3^2").formatted, "512");
});

test("supports constants and functions", () => {
  assert.equal(evaluate("round(pi)").formatted, "3");
  assert.equal(evaluate("sqrt(81)").formatted, "9");
  assert.equal(evaluate("avg(2;4;6)").formatted, "4");
});

test("supports multi-line variables and summary tokens", () => {
  const lines = evaluateDocument("x = 10\ny = x * 2\nprev + sum + avg");

  assert.equal(lines[0].formatted, "10");
  assert.equal(lines[1].formatted, "20");
  assert.equal(lines[2].formatted, "65");
});

test("supports basic percentage expressions", () => {
  assert.equal(evaluate("20% of 50").formatted, "10");
  assert.equal(evaluate("1 + 5%").formatted, "1.05");
});

test("converts basic units", () => {
  assert.equal(evaluate("20 inches in cm").formatted, "50.8 cm");
  assert.equal(evaluate("32px in rem").formatted, "2 rem");
  assert.equal(evaluate("2 hours in minutes").formatted, "120 min");
});

test("reports parse errors without throwing from document evaluation", () => {
  const [line] = evaluateDocument("unknown + 1");

  assert.equal(line.ok, false);
  assert.match(line.error, /Unknown variable/);
});
