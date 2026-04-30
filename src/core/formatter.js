const DEFAULT_FORMAT_OPTIONS = {
  maximumFractionDigits: 12,
  minimumFractionDigits: 0,
  useGrouping: false,
  scientificNotationThreshold: 1e15,
};

export function formatResult(result, options = {}) {
  if (!result) {
    return "";
  }

  if (result.type === "date") {
    return result.value;
  }
  if (result.type === "duration") {
    const value = formatNumber(result.value, options);
    return `${value} ${Math.abs(result.value) === 1 ? "day" : "days"}`;
  }

  const value = formatNumber(result.value, options);
  if (!result.unitId) {
    return value;
  }
  if (result.unitId === "%") {
    return `${value}%`;
  }
  return `${value} ${result.unitId}`;
}

export function formatNumber(value, options = {}) {
  if (!Number.isFinite(value)) {
    return String(value);
  }

  const resolved = resolveFormatOptions(options);
  const rounded = Math.abs(value) < 1e-12 ? 0 : value;

  if (Math.abs(rounded) >= resolved.scientificNotationThreshold) {
    return rounded.toExponential();
  }

  return new Intl.NumberFormat("en-US", {
    maximumFractionDigits: resolved.maximumFractionDigits,
    minimumFractionDigits: resolved.minimumFractionDigits,
    useGrouping: resolved.useGrouping,
  }).format(rounded);
}

function resolveFormatOptions(options) {
  const maximumFractionDigits = options.precision ?? options.maximumFractionDigits ?? DEFAULT_FORMAT_OPTIONS.maximumFractionDigits;
  const minimumFractionDigits = options.minimumFractionDigits ?? DEFAULT_FORMAT_OPTIONS.minimumFractionDigits;

  return {
    maximumFractionDigits,
    minimumFractionDigits,
    useGrouping: options.useGrouping ?? DEFAULT_FORMAT_OPTIONS.useGrouping,
    scientificNotationThreshold: options.scientificNotationThreshold
      ?? DEFAULT_FORMAT_OPTIONS.scientificNotationThreshold,
  };
}
