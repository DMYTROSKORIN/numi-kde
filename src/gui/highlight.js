import { findUnit } from "../core/units.js";

const TOKEN_PATTERN = /\s+|\d{1,3}(?:,\d{3})+(?:\.\d+)?(?:e[+-]?\d+)?|\d+(?:[.,]\d+)?(?:e[+-]?\d+)?|[A-Za-z_π][\wπ]*|:=|[%+\-*/^(),;=]|\S/giu;

const OPERATOR_WORDS = new Set(["as", "from", "in", "of", "to"]);
const OPERATOR_SYMBOLS = new Set([":=", "+", "-", "*", "/", "^", "(", ")", ",", ";", "="]);

const HIGHLIGHT_COLORS = {
  entity: "#6fc4e8",
  operator: "#ffd35a",
};

export function createHighlightedHtml(line) {
  return Array.from(line.matchAll(TOKEN_PATTERN), ([token]) => {
    if (/^\s+$/u.test(token)) {
      return escapeWhitespace(token);
    }

    const kind = classifyHighlightToken(token);
    const escaped = escapeHtml(token);
    if (!kind) {
      return escaped;
    }

    return `<span style="color:${HIGHLIGHT_COLORS[kind]}">${escaped}</span>`;
  }).join("");
}

export function classifyHighlightToken(token) {
  const lower = token.toLowerCase();

  if (OPERATOR_WORDS.has(lower) || OPERATOR_SYMBOLS.has(token)) {
    return "operator";
  }
  if (token === "%" || findUnit(token) || /^[A-Z]{3,5}$/u.test(token)) {
    return "entity";
  }

  return null;
}

function escapeHtml(input) {
  return input
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;");
}

function escapeWhitespace(input) {
  return input
    .replaceAll("\t", "&nbsp;&nbsp;&nbsp;&nbsp;")
    .replaceAll(" ", "&nbsp;");
}
