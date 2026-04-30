import { NumiError } from "./errors.js";

export function parseLine(rawLine) {
  const stripped = stripComment(rawLine);
  const leadingWhitespace = stripped.match(/^\s*/u)?.[0].length ?? 0;
  const source = stripped.trim();

  if (!source) {
    return {
      executable: false,
      source,
      tokens: [],
      ast: null,
      assignment: null,
    };
  }

  const tokens = tokenize(source, leadingWhitespace);
  const assignment = parseAssignment(source, leadingWhitespace);
  const expression = assignment ? assignment.expression : source;
  const expressionOffset = assignment ? assignment.expressionRange.start : leadingWhitespace;

  return {
    executable: true,
    source,
    tokens: tokens.filter((token) => token.type !== "eof"),
    ast: parseExpression(expression, expressionOffset),
    assignment,
  };
}

export function parseExpression(input, offset = 0) {
  const parser = new Parser(tokenize(input, offset));
  const ast = parser.parseExpression();
  parser.expect("eof");
  return ast;
}

export function tokenize(input, offset = 0) {
  const tokens = [];
  let index = 0;

  while (index < input.length) {
    const char = input[index];

    if (/\s/u.test(char)) {
      index += 1;
      continue;
    }

    const number = matchNumber(input.slice(index));
    if (number) {
      tokens.push({
        type: "number",
        value: parseNumberLiteral(number[0]),
        raw: number[0],
        start: offset + index,
        end: offset + index + number[0].length,
      });
      index += number[0].length;
      continue;
    }

    const identifier = input.slice(index).match(/^[A-Za-z_π\p{L}][\wπ\p{L}]*/u);
    if (identifier) {
      const value = identifier[0].toLowerCase();
      if (value === "of") {
        tokens.push({
          type: "*",
          value: "*",
          raw: identifier[0],
          start: offset + index,
          end: offset + index + identifier[0].length,
        });
      } else {
        tokens.push({
          type: "identifier",
          value: identifier[0],
          raw: identifier[0],
          start: offset + index,
          end: offset + index + identifier[0].length,
        });
      }
      index += identifier[0].length;
      continue;
    }

    if (char === ":" && input[index + 1] === "=") {
      tokens.push({
        type: "assignment",
        value: ":=",
        raw: ":=",
        start: offset + index,
        end: offset + index + 2,
      });
      index += 2;
      continue;
    }

    if ("+-*/^(),;%=".includes(char)) {
      tokens.push({
        type: char === "%" ? "percent" : char === ";" ? "," : char === "=" ? "assignment" : char,
        value: char,
        raw: char,
        start: offset + index,
        end: offset + index + 1,
      });
      index += 1;
      continue;
    }

    throw new NumiError(`Unexpected character: ${char}`, { start: offset + index, end: offset + index + 1 });
  }

  tokens.push({ type: "eof", value: "", raw: "", start: offset + input.length, end: offset + input.length });
  return tokens;
}

export function stripComment(line) {
  const hashIndex = line.indexOf("#");
  const slashIndex = line.indexOf("//");
  const indexes = [hashIndex, slashIndex].filter((index) => index >= 0);
  return indexes.length > 0 ? line.slice(0, Math.min(...indexes)) : line;
}

function parseAssignment(source, offset) {
  const match = source.match(/^([A-Za-z_][\w]*)\s*(?::=|=)\s*(.+)$/u);
  if (!match) {
    return null;
  }

  const operatorStart = match[1].length + (source.slice(match[1].length).match(/^\s*/u)?.[0].length ?? 0);
  const operator = source.startsWith(":=", operatorStart) ? ":=" : "=";
  const expressionStart = operatorStart + operator.length
    + (source.slice(operatorStart + operator.length).match(/^\s*/u)?.[0].length ?? 0);

  return {
    name: match[1],
    operator,
    range: { start: offset, end: offset + match[1].length },
    operatorRange: { start: offset + operatorStart, end: offset + operatorStart + operator.length },
    expression: source.slice(expressionStart),
    expressionRange: { start: offset + expressionStart, end: offset + source.length },
  };
}

function matchNumber(input) {
  return input.match(/^\d{1,3}(?:,\d{3})+(?:\.\d+)?(?:e[+-]?\d+)?/iu)
    ?? input.match(/^\d+(?:[.,]\d+)?(?:e[+-]?\d+)?/iu);
}

function parseNumberLiteral(input) {
  if (/^\d{1,3}(?:,\d{3})+(?:\.\d+)?(?:e[+-]?\d+)?$/iu.test(input)) {
    return Number(input.replaceAll(",", ""));
  }
  return Number(input.replace(",", "."));
}

class Parser {
  constructor(tokens) {
    this.tokens = tokens;
    this.position = 0;
  }

  parseExpression() {
    let node = this.parseTerm();

    while (this.match("+") || this.match("-")) {
      const operator = this.previous();
      const right = this.parseTerm();
      node = binaryNode(operator.type, node, right, operator, false);
    }

    return node;
  }

  parseTerm() {
    let node = this.parseUnary();

    while (true) {
      if (this.match("*") || this.match("/")) {
        const operator = this.previous();
        const right = this.parseUnary();
        node = binaryNode(operator.type, node, right, operator, false);
        continue;
      }
      if (this.startsImplicitMultiplication()) {
        const right = this.parseUnary();
        node = binaryNode("*", node, right, null, true);
        continue;
      }
      break;
    }

    return node;
  }

  parsePower() {
    let node = this.parsePostfix();

    if (this.match("^")) {
      const operator = this.previous();
      const right = this.parseUnary();
      node = binaryNode("^", node, right, operator, false);
    }

    return node;
  }

  parseUnary() {
    if (this.match("+")) {
      const operator = this.previous();
      const argument = this.parsePower();
      return unaryNode("+", argument, operator);
    }
    if (this.match("-")) {
      const operator = this.previous();
      const argument = this.parsePower();
      return unaryNode("-", argument, operator);
    }

    return this.parsePower();
  }

  parsePostfix() {
    let node = this.parsePrimary();

    while (this.match("percent")) {
      const operator = this.previous();
      node = {
        type: "PercentExpression",
        argument: node,
        operatorRange: rangeFromToken(operator),
        range: { start: node.range.start, end: operator.end },
      };
    }

    return node;
  }

  parsePrimary() {
    if (this.match("number")) {
      const token = this.previous();
      return {
        type: "NumberLiteral",
        value: token.value,
        raw: token.raw,
        range: rangeFromToken(token),
      };
    }

    if (this.match("identifier")) {
      const token = this.previous();
      if (this.match("(")) {
        const open = this.previous();
        return this.callNode(token, open, this.parseArguments());
      }

      return {
        type: "Identifier",
        name: token.value,
        range: rangeFromToken(token),
      };
    }

    if (this.match("(")) {
      const open = this.previous();
      const expression = this.parseExpression();
      const close = this.expect(")");
      return {
        type: "GroupExpression",
        expression,
        range: { start: open.start, end: close.end },
      };
    }

    throw new NumiError(`Expected expression, got ${this.peek().type}`, this.peekRange());
  }

  parseArguments() {
    const values = [];

    if (!this.check(")")) {
      do {
        values.push(this.parseExpression());
      } while (this.match(","));
    }

    const close = this.expect(")");
    return { values, close };
  }

  callNode(identifier, open, args) {
    return {
      type: "CallExpression",
      callee: {
        type: "Identifier",
        name: identifier.value,
        range: rangeFromToken(identifier),
      },
      arguments: args.values,
      range: { start: identifier.start, end: args.close.end },
      openRange: rangeFromToken(open),
      closeRange: rangeFromToken(args.close),
    };
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
    throw new NumiError(`Expected ${type}, got ${this.peek().type}`, this.peekRange());
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

  startsImplicitMultiplication() {
    return ["number", "identifier", "("].includes(this.peek().type);
  }

  peekRange() {
    return rangeFromToken(this.peek());
  }
}

function binaryNode(operator, left, right, operatorToken, implicit) {
  return {
    type: "BinaryExpression",
    operator,
    left,
    right,
    implicit,
    operatorRange: operatorToken ? rangeFromToken(operatorToken) : null,
    range: { start: left.range.start, end: right.range.end },
  };
}

function unaryNode(operator, argument, operatorToken) {
  return {
    type: "UnaryExpression",
    operator,
    argument,
    operatorRange: rangeFromToken(operatorToken),
    range: { start: operatorToken.start, end: argument.range.end },
  };
}

function rangeFromToken(token) {
  return { start: token.start, end: token.end };
}
