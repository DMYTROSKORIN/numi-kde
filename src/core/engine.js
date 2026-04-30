import { evaluateDateExpression } from "./dates.js";
import { createDiagnostic, NumiError } from "./errors.js";
import { createExtensionRegistry, loadExtensionModules } from "./extensions.js";
import { parseExpression, parseLine } from "./syntax.js";
import { convertQuantity, createQuantity, findUnit } from "./units.js";
export { formatResult } from "./formatter.js";
import { formatResult } from "./formatter.js";
export { NumiError } from "./errors.js";
export { createExtensionRegistry, loadExtensionModules } from "./extensions.js";

const CONSTANTS = new Map([
  ["pi", Math.PI],
  ["π", Math.PI],
  ["tau", Math.PI * 2],
  ["e", Math.E],
]);

const FUNCTIONS = new Map([
  ["abs", ([x]) => Math.abs(x)],
  ["ceil", ([x]) => Math.ceil(x)],
  ["floor", ([x]) => Math.floor(x)],
  ["round", ([x]) => Math.round(x)],
  ["sqrt", ([x]) => Math.sqrt(x)],
  ["sin", ([x]) => Math.sin(x)],
  ["cos", ([x]) => Math.cos(x)],
  ["tan", ([x]) => Math.tan(x)],
  ["ln", ([x]) => Math.log(x)],
  ["log", (values) => values.length === 1 ? Math.log10(values[0]) : Math.log(values[1]) / Math.log(values[0])],
  ["min", (values) => Math.min(...values)],
  ["max", (values) => Math.max(...values)],
  ["sum", (values) => values.reduce((sum, value) => sum + value, 0)],
  ["avg", (values) => values.reduce((sum, value) => sum + value, 0) / values.length],
]);

export function evaluateDocument(source, options = {}) {
  const extensions = options.extensions ?? createExtensionRegistry(options.setupExtensions);
  const state = {
    variables: new Map([...CONSTANTS, ...extensions.variables]),
    numericResults: [],
    options,
    extensions,
  };

  return source.split(/\r?\n/).map((line, index) => evaluateLine(line, state, index));
}

export function parseDocument(source) {
  return source.split(/\r?\n/).map((line, index) => {
    try {
      const parsed = parseLine(line);
      return {
        ok: true,
        line: index + 1,
        input: line,
        tokens: parsed.tokens,
        ast: parsed.ast,
        assignment: parsed.assignment,
        diagnostics: [],
      };
    } catch (error) {
      const diagnostic = createDiagnostic(error);
      return {
        ok: false,
        line: index + 1,
        input: line,
        tokens: [],
        diagnostics: [diagnostic],
      };
    }
  });
}

export function evaluate(input, options = {}) {
  const results = evaluateDocument(input, options);
  for (let i = results.length - 1; i >= 0; i -= 1) {
    if (results[i].ok && results[i].result !== null) {
      return results[i];
    }
  }
  return results.at(-1) ?? { ok: true, input, result: null, formatted: "" };
}

const HELP_TEXT = "10+5 · A:=5 · 10 km в mi · сегодня+2 нед · 20% от 100";

function evaluateLine(rawLine, state, index) {
  if (rawLine.trim() === "/help") {
    return { ok: true, line: index + 1, input: rawLine, result: { value: "/help" }, formatted: HELP_TEXT, diagnostics: [] };
  }

  let parsed;
  try {
    parsed = parseLine(rawLine);
  } catch (error) {
    return { ok: false, line: index + 1, input: rawLine, result: null, formatted: "", diagnostics: [createDiagnostic(error)] };
  }
  const input = parsed.source;

  if (!parsed.executable) {
    return { ok: true, line: index + 1, input: rawLine, result: null, formatted: "", diagnostics: [] };
  }

  try {
    const assignment = parsed.assignment;
    const expression = assignment ? assignment.expression : input;
    const result = evaluateExpression(expression, state);

    if (assignment) {
      state.variables.set(assignment.name, result.value);
    }

    state.variables.set("prev", result.value);
    if (typeof result.value === "number") {
      state.numericResults.push(result.value);
      state.variables.set("sum", state.numericResults.reduce((sum, value) => sum + value, 0));
      state.variables.set("avg", state.variables.get("sum") / state.numericResults.length);
    }

    return {
      ok: true,
      line: index + 1,
      input: rawLine,
      result,
      formatted: formatResult(result, state.options.format),
      diagnostics: [],
    };
  } catch (error) {
    const diagnostic = createDiagnostic(error);
    return {
      ok: false,
      line: index + 1,
      input: rawLine,
      error: diagnostic.message,
      formatted: "",
      diagnostics: [diagnostic],
    };
  }
}

function evaluateExpression(input, state) {
  const dateResult = evaluateDateExpression(input);
  if (dateResult) {
    return dateResult;
  }

  const percentageRatio = input.match(/^(.+?)\s+as\s+%\s+of\s+(.+)$/iu);
  if (percentageRatio) {
    const value = parseMath(percentageRatio[1], state);
    const total = parseMath(percentageRatio[2], state);
    return { value: value / total * 100, unitId: "%", dimension: null };
  }

  const percentageChange = input.match(/^from\s+(.+?)\s+to\s+(.+)$/iu);
  if (percentageChange) {
    const start = parseMath(percentageChange[1], state);
    const end = parseMath(percentageChange[2], state);
    return { value: (end - start) / start * 100, unitId: "%", dimension: null };
  }

  const percentageAddSub = input.match(/^(.+)\s*([+-])\s*([^+-]+%)$/u);
  if (percentageAddSub) {
    const base = parseMath(percentageAddSub[1], state);
    const percent = parseMath(percentageAddSub[3], state);
    return {
      value: percentageAddSub[2] === "+" ? base + base * percent : base - base * percent,
      unitId: null,
      dimension: null,
    };
  }

  const conversion = splitConversion(input);

  if (conversion) {
    const quantity = evaluateQuantity(conversion.left, state);
    const targetUnit = findUnit(conversion.target, state.extensions.units);
    return convertQuantity(quantity, targetUnit);
  }

  return evaluateQuantity(input, state);
}

function splitConversion(input) {
  const match = input.match(/^(.+?)\s+(?:in|to|as)\s+([A-Za-z°%][A-Za-z0-9°%./-]*)$/iu);
  if (!match) {
    return null;
  }

  return {
    left: match[1].trim(),
    target: match[2].trim(),
  };
}

function evaluateQuantity(input, state) {
  const unitMatch = input.match(/^(.+?)(?:\s+|)([A-Za-z°%][A-Za-z0-9°%./-]*)$/u);

  if (unitMatch) {
    const unit = findUnit(unitMatch[2], state.extensions.units);
    if (unit) {
      const value = parseMath(unitMatch[1], state);
      return createQuantity(value, unit);
    }
  }

  return { value: parseMath(input, state), unitId: null, dimension: null };
}

function parseMath(input, state) {
  return evaluateAst(parseExpression(input), state);
}

function evaluateAst(node, state) {
  switch (node.type) {
    case "NumberLiteral":
      return node.value;
    case "Identifier":
      return resolveIdentifier(node, state);
    case "GroupExpression":
      return evaluateAst(node.expression, state);
    case "UnaryExpression": {
      const value = evaluateAst(node.argument, state);
      return node.operator === "-" ? -value : value;
    }
    case "PercentExpression":
      return evaluateAst(node.argument, state) / 100;
    case "BinaryExpression":
      return evaluateBinary(node, state);
    case "CallExpression":
      return evaluateCall(node, state);
    default:
      throw new NumiError(`Unknown AST node: ${node.type}`, node.range ?? null);
  }
}

function evaluateBinary(node, state) {
  const left = evaluateAst(node.left, state);
  const right = evaluateAst(node.right, state);

  switch (node.operator) {
    case "+":
      return left + right;
    case "-":
      return left - right;
    case "*":
      return left * right;
    case "/":
      return left / right;
    case "^":
      return left ** right;
    default:
      throw new NumiError(`Unknown operator: ${node.operator}`, node.operatorRange);
  }
}

function evaluateCall(node, state) {
  const fn = state.extensions.functions.get(node.callee.name.toLowerCase())
    ?? FUNCTIONS.get(node.callee.name.toLowerCase());
  if (!fn) {
    throw new NumiError(`Unknown function: ${node.callee.name}`, node.callee.range);
  }
  return fn(node.arguments.map((argument) => evaluateAst(argument, state)));
}

function resolveIdentifier(node, state) {
  if (state.variables.has(node.name)) {
    return state.variables.get(node.name);
  }
  if (state.variables.has(node.name.toLowerCase())) {
    return state.variables.get(node.name.toLowerCase());
  }

  throw new NumiError(`Unknown variable: ${node.name}`, node.range);
}
