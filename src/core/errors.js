export class NumiError extends Error {
  constructor(message, range = null) {
    super(message);
    this.name = "NumiError";
    this.range = range;
  }
}

export function createDiagnostic(error) {
  return {
    severity: "error",
    message: error instanceof Error ? error.message : String(error),
    range: error instanceof NumiError ? error.range : null,
  };
}
