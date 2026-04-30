const CONSTANTS = new Map([
  ["pi", Math.PI],
  ["π", Math.PI],
  ["tau", Math.PI * 2],
  ["e", Math.E],
]);

const UNIT_DEFS = [
  ["m", "m", 1, ["m", "meter", "meters", "metre", "metres"]],
  ["cm", "m", 0.01, ["cm", "centimeter", "centimeters", "centimetre", "centimetres"]],
  ["mm", "m", 0.001, ["mm", "millimeter", "millimeters", "millimetre", "millimetres"]],
  ["km", "m", 1000, ["km", "kilometer", "kilometers", "kilometre", "kilometres"]],
  ["in", "m", 0.0254, ["in", "inch", "inches"]],
  ["ft", "m", 0.3048, ["ft", "foot", "feet"]],
  ["yd", "m", 0.9144, ["yd", "yard", "yards"]],
  ["mi", "m", 1609.344, ["mi", "mile", "miles"]],
  ["s", "s", 1, ["s", "sec", "second", "seconds"]],
  ["min", "s", 60, ["min", "minute", "minutes"]],
  ["h", "s", 3600, ["h", "hr", "hour", "hours"]],
  ["day", "s", 86400, ["day", "days"]],
  ["px", "px", 1, ["px", "pixel", "pixels"]],
  ["rem", "px", 16, ["rem"]],
];

const UNITS = new Map();
const UNIT_ALIASES = new Map();

for (const [id, dimension, ratio, aliases] of UNIT_DEFS) {
  const unit = { id, dimension, ratio };
  UNITS.set(id, unit);
  for (const alias of aliases) {
    UNIT_ALIASES.set(alias.toLowerCase(), unit);
  }
}

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

export class NumiError extends Error {
  constructor(message) {
    super(message);
    this.name = "NumiError";
  }
}

export function evaluateDocument(source) {
  const state = {
    variables: new Map(CONSTANTS),
    numericResults: [],
  };

  return source.split(/\r?\n/).map((line, index) => evaluateLine(line, state, index));
}

export function evaluate(input) {
  const results = evaluateDocument(input);
  for (let i = results.length - 1; i >= 0; i -= 1) {
    if (results[i].ok && results[i].result !== null) {
      return results[i];
    }
  }
  return results.at(-1) ?? { ok: true, input, result: null, formatted: "" };
}

function evaluateLine(rawLine, state, index) {
  const input = rawLine.trim();

  if (!input || input.startsWith("#") || input.startsWith("//")) {
    return { ok: true, line: index + 1, input: rawLine, result: null, formatted: "" };
  }

  try {
    const assignment = input.match(/^([A-Za-z_][\w]*)\s*=\s*(.+)$/u);
    const expression = assignment ? assignment[2] : input;
    const result = evaluateExpression(expression, state);

    if (assignment) {
      state.variables.set(assignment[1], result.value);
    }

    state.variables.set("prev", result.value);
    state.numericResults.push(result.value);
    state.variables.set("sum", state.numericResults.reduce((sum, value) => sum + value, 0));
    state.variables.set("avg", state.variables.get("sum") / state.numericResults.length);

    return {
      ok: true,
      line: index + 1,
      input: rawLine,
      result,
      formatted: formatResult(result),
    };
  } catch (error) {
    return {
      ok: false,
      line: index + 1,
      input: rawLine,
      error: error instanceof Error ? error.message : String(error),
      formatted: "",
    };
  }
}

function evaluateExpression(input, state) {
  const conversion = splitConversion(input);

  if (conversion) {
    const quantity = evaluateQuantity(conversion.left, state);
    const targetUnit = findUnit(conversion.target);
    return convertQuantity(quantity, targetUnit);
  }

  return evaluateQuantity(input, state);
}

function splitConversion(input) {
  const match = input.match(/^(.+?)\s+(?:in|to|as)\s+([A-Za-z%][A-Za-z0-9%./-]*)$/iu);
  if (!match) {
    return null;
  }

  return {
    left: match[1].trim(),
    target: match[2].trim(),
  };
}

function evaluateQuantity(input, state) {
  const unitMatch = input.match(/^(.+?)(?:\s+|)([A-Za-z%][A-Za-z0-9%./-]*)$/u);

  if (unitMatch) {
    const unit = findUnit(unitMatch[2]);
    if (unit) {
      const value = parseMath(unitMatch[1], state);
      return { value: value * unit.ratio, unitId: unit.id, dimension: unit.dimension };
    }
  }

  return { value: parseMath(input, state), unitId: null, dimension: null };
}

function convertQuantity(quantity, targetUnit) {
  if (!targetUnit) {
    throw new NumiError("Unknown target unit");
  }
  if (!quantity.dimension) {
    throw new NumiError("Cannot convert a unitless value");
  }
  if (quantity.dimension !== targetUnit.dimension) {
    throw new NumiError(`Cannot convert ${quantity.unitId} to ${targetUnit.id}`);
  }

  return {
    value: quantity.value / targetUnit.ratio,
    unitId: targetUnit.id,
    dimension: targetUnit.dimension,
  };
}

function findUnit(input) {
  return UNIT_ALIASES.get(input.toLowerCase()) ?? UNITS.get(input.toLowerCase()) ?? null;
}

function parseMath(input, state) {
  const parser = new Parser(tokenize(input), state);
  const value = parser.parseExpression();
  parser.expect("eof");
  return value;
}

function tokenize(input) {
  const tokens = [];
  let index = 0;

  while (index < input.length) {
    const char = input[index];

    if (/\s/u.test(char)) {
      index += 1;
      continue;
    }

    const number = input.slice(index).match(/^\d+(?:[.,]\d+)?(?:e[+-]?\d+)?/iu);
    if (number) {
      tokens.push({ type: "number", value: Number(number[0].replace(",", ".")) });
      index += number[0].length;
      continue;
    }

    const identifier = input.slice(index).match(/^[A-Za-z_π][\wπ]*/u);
    if (identifier) {
      const value = identifier[0].toLowerCase();
      if (value === "of") {
        tokens.push({ type: "*", value: "*" });
      } else {
        tokens.push({ type: "identifier", value: identifier[0] });
      }
      index += identifier[0].length;
      continue;
    }

    if ("+-*/^(),;%".includes(char)) {
      tokens.push({ type: char === "%" ? "percent" : char === ";" ? "," : char, value: char });
      index += 1;
      continue;
    }

    throw new NumiError(`Unexpected character: ${char}`);
  }

  tokens.push({ type: "eof", value: "" });
  return tokens;
}

class Parser {
  constructor(tokens, state) {
    this.tokens = tokens;
    this.position = 0;
    this.state = state;
  }

  parseExpression() {
    let value = this.parseTerm();

    while (this.match("+") || this.match("-")) {
      const operator = this.previous().type;
      const right = this.parseTerm();
      value = operator === "+" ? value + right : value - right;
    }

    return value;
  }

  parseTerm() {
    let value = this.parsePower();

    while (this.match("*") || this.match("/")) {
      const operator = this.previous().type;
      const right = this.parsePower();
      value = operator === "*" ? value * right : value / right;
    }

    return value;
  }

  parsePower() {
    let value = this.parseUnary();

    if (this.match("^")) {
      value = value ** this.parsePower();
    }

    return value;
  }

  parseUnary() {
    if (this.match("+")) {
      return this.parseUnary();
    }
    if (this.match("-")) {
      return -this.parseUnary();
    }

    return this.parsePostfix();
  }

  parsePostfix() {
    let value = this.parsePrimary();

    while (this.match("percent")) {
      value /= 100;
    }

    return value;
  }

  parsePrimary() {
    if (this.match("number")) {
      return this.previous().value;
    }

    if (this.match("identifier")) {
      const identifier = this.previous().value;
      if (this.match("(")) {
        return this.callFunction(identifier, this.parseArguments());
      }

      if (this.state.variables.has(identifier)) {
        return this.state.variables.get(identifier);
      }
      if (this.state.variables.has(identifier.toLowerCase())) {
        return this.state.variables.get(identifier.toLowerCase());
      }

      throw new NumiError(`Unknown variable: ${identifier}`);
    }

    if (this.match("(")) {
      const value = this.parseExpression();
      this.expect(")");
      return value;
    }

    throw new NumiError(`Expected expression, got ${this.peek().type}`);
  }

  parseArguments() {
    const values = [];

    if (!this.check(")")) {
      do {
        values.push(this.parseExpression());
      } while (this.match(","));
    }

    this.expect(")");
    return values;
  }

  callFunction(identifier, values) {
    const fn = FUNCTIONS.get(identifier.toLowerCase());
    if (!fn) {
      throw new NumiError(`Unknown function: ${identifier}`);
    }
    return fn(values);
  }

  match(type) {
    if (!this.check(type)) {
      return false;
    }
    this.position += 1;
    return true;
  }

  expect(type) {
    if (this.match(type)) {
      return this.previous();
    }
    throw new NumiError(`Expected ${type}, got ${this.peek().type}`);
  }

  check(type) {
    return this.peek().type === type;
  }

  peek() {
    return this.tokens[this.position];
  }

  previous() {
    return this.tokens[this.position - 1];
  }
}

export function formatResult(result) {
  if (!result) {
    return "";
  }

  const value = formatNumber(result.value);
  return result.unitId ? `${value} ${result.unitId}` : value;
}

function formatNumber(value) {
  if (!Number.isFinite(value)) {
    return String(value);
  }

  const rounded = Math.abs(value) < 1e-12 ? 0 : value;
  return new Intl.NumberFormat("en-US", {
    maximumFractionDigits: 12,
    useGrouping: false,
  }).format(rounded);
}
