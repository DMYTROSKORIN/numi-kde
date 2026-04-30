function linearUnit(id, dimension, ratio, aliases) {
  return {
    id,
    dimension,
    aliases,
    toBase: (value) => value * ratio,
    fromBase: (value) => value / ratio,
  };
}

export function createLinearUnit(id, dimension, ratio, aliases = [id]) {
  return linearUnit(id, dimension, ratio, aliases);
}

function offsetUnit(id, dimension, toBase, fromBase, aliases) {
  return { id, dimension, aliases, toBase, fromBase };
}

const UNIT_DEFS = [
  linearUnit("mm", "length", 0.001, ["mm", "millimeter", "millimeters", "millimetre", "millimetres"]),
  linearUnit("cm", "length", 0.01, ["cm", "centimeter", "centimeters", "centimetre", "centimetres"]),
  linearUnit("m", "length", 1, ["m", "meter", "meters", "metre", "metres"]),
  linearUnit("km", "length", 1000, ["km", "kilometer", "kilometers", "kilometre", "kilometres"]),
  linearUnit("in", "length", 0.0254, ["in", "inch", "inches"]),
  linearUnit("ft", "length", 0.3048, ["ft", "foot", "feet"]),
  linearUnit("yd", "length", 0.9144, ["yd", "yard", "yards"]),
  linearUnit("mi", "length", 1609.344, ["mi", "mile", "miles"]),

  linearUnit("mg", "mass", 0.000001, ["mg", "milligram", "milligrams"]),
  linearUnit("g", "mass", 0.001, ["g", "gram", "grams"]),
  linearUnit("kg", "mass", 1, ["kg", "kilogram", "kilograms"]),
  linearUnit("t", "mass", 1000, ["t", "ton", "tons", "tonne", "tonnes"]),
  linearUnit("oz", "mass", 0.028349523125, ["oz", "ounce", "ounces"]),
  linearUnit("lb", "mass", 0.45359237, ["lb", "lbs", "pound", "pounds"]),

  linearUnit("ms", "time", 0.001, ["ms", "millisecond", "milliseconds"]),
  linearUnit("s", "time", 1, ["s", "sec", "second", "seconds"]),
  linearUnit("min", "time", 60, ["min", "minute", "minutes"]),
  linearUnit("h", "time", 3600, ["h", "hr", "hour", "hours"]),
  linearUnit("day", "time", 86400, ["day", "days"]),
  linearUnit("week", "time", 604800, ["week", "weeks"]),

  linearUnit("bit", "data", 1, ["bit", "bits", "b"]),
  linearUnit("byte", "data", 8, ["byte", "bytes", "B"]),
  linearUnit("KB", "data", 8000, ["kb", "kilobyte", "kilobytes"]),
  linearUnit("MB", "data", 8000000, ["mb", "megabyte", "megabytes"]),
  linearUnit("GB", "data", 8000000000, ["gb", "gigabyte", "gigabytes"]),
  linearUnit("KiB", "data", 8192, ["kib", "kibibyte", "kibibytes"]),
  linearUnit("MiB", "data", 8388608, ["mib", "mebibyte", "mebibytes"]),
  linearUnit("GiB", "data", 8589934592, ["gib", "gibibyte", "gibibytes"]),

  linearUnit("px", "css", 1, ["px", "pixel", "pixels"]),
  linearUnit("rem", "css", 16, ["rem"]),

  offsetUnit("K", "temperature", (value) => value, (value) => value, ["k", "kelvin"]),
  offsetUnit("C", "temperature", (value) => value + 273.15, (value) => value - 273.15, [
    "c",
    "celsius",
    "degc",
    "°c",
  ]),
  offsetUnit("F", "temperature", (value) => (value - 32) * 5 / 9 + 273.15, (value) => (value - 273.15) * 9 / 5 + 32, [
    "f",
    "fahrenheit",
    "degf",
    "°f",
  ]),
];

export const UNITS = new Map();
const UNIT_ALIASES = new Map();
const LOWER_UNIT_ALIASES = new Map();

for (const unit of UNIT_DEFS) {
  UNITS.set(unit.id.toLowerCase(), unit);
  for (const alias of unit.aliases) {
    UNIT_ALIASES.set(alias, unit);
    LOWER_UNIT_ALIASES.set(alias.toLowerCase(), unit);
  }
}

export function findUnit(input, extensionUnits = []) {
  const extensionUnit = findExtensionUnit(input, extensionUnits);
  return extensionUnit
    ?? UNIT_ALIASES.get(input)
    ?? LOWER_UNIT_ALIASES.get(input.toLowerCase())
    ?? UNITS.get(input.toLowerCase())
    ?? null;
}

export function convertQuantity(quantity, targetUnit) {
  if (!targetUnit) {
    throw new Error("Unknown target unit");
  }
  if (!quantity.dimension) {
    throw new Error("Cannot convert a unitless value");
  }
  if (quantity.dimension !== targetUnit.dimension) {
    throw new Error(`Cannot convert ${quantity.unitId} to ${targetUnit.id}`);
  }

  const baseValue = quantity.baseValue ?? quantity.value;
  return {
    value: targetUnit.fromBase(baseValue),
    baseValue,
    unitId: targetUnit.id,
    dimension: targetUnit.dimension,
  };
}

export function createQuantity(value, unit) {
  return {
    value,
    baseValue: unit.toBase(value),
    unitId: unit.id,
    dimension: unit.dimension,
  };
}

function findExtensionUnit(input, units) {
  for (const unit of units) {
    if (unit.id === input || unit.aliases.includes(input)) {
      return unit;
    }
    if (unit.id.toLowerCase() === input.toLowerCase()) {
      return unit;
    }
    if (unit.aliases.some((alias) => alias.toLowerCase() === input.toLowerCase())) {
      return unit;
    }
  }
  return null;
}
