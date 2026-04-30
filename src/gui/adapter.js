import { evaluateDocument, parseDocument } from "../core/engine.js";
import { createHighlightedHtml } from "./highlight.js";

export function createDocumentViewModel(source, options = {}) {
  const evaluated = evaluateDocument(source, options);
  const parsed = parseDocument(source);
  const lines = evaluated.map((line, index) => {
    const parsedLine = parsed[index] ?? {};
    const diagnostics = [...(line.diagnostics ?? []), ...(parsedLine.diagnostics ?? [])];

    return {
      number: index + 1,
      input: line.input,
      ok: line.ok,
      result: line.formatted,
      diagnostics,
      tokens: parsedLine.tokens ?? [],
      highlightedHtml: createHighlightedHtml(line.input),
      astType: parsedLine.ast?.type ?? null,
      assignment: parsedLine.assignment?.name ?? null,
    };
  });

  return {
    source,
    lines,
    summary: {
      lineCount: lines.length,
      resultCount: lines.filter((line) => line.result.length > 0).length,
      errorCount: lines.filter((line) => !line.ok || line.diagnostics.length > 0).length,
    },
  };
}
