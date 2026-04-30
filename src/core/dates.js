const DAY_MS = 24 * 60 * 60 * 1000;

const DURATION_UNITS = new Map([
  ["day", 1],
  ["days", 1],
  ["week", 7],
  ["weeks", 7],
]);

export function evaluateDateExpression(input) {
  const dateDiff = input.match(/^(\d{4}-\d{2}-\d{2})\s*-\s*(\d{4}-\d{2}-\d{2})$/u);
  if (dateDiff) {
    const end = parseIsoDate(dateDiff[1]);
    const start = parseIsoDate(dateDiff[2]);
    return {
      type: "duration",
      value: Math.round((end.getTime() - start.getTime()) / DAY_MS),
      unitId: "days",
      dimension: "date-duration",
    };
  }

  const dateMath = input.match(/^(\d{4}-\d{2}-\d{2})\s*([+-])\s*(\d+(?:[.,]\d+)?)\s+([A-Za-z]+)$/u);
  if (dateMath) {
    const start = parseIsoDate(dateMath[1]);
    const sign = dateMath[2] === "+" ? 1 : -1;
    const amount = Number(dateMath[3].replace(",", "."));
    const multiplier = DURATION_UNITS.get(dateMath[4].toLowerCase());

    if (!multiplier) {
      return null;
    }

    return {
      type: "date",
      value: formatIsoDate(new Date(start.getTime() + sign * amount * multiplier * DAY_MS)),
      unitId: null,
      dimension: "date",
    };
  }

  if (/^\d{4}-\d{2}-\d{2}$/u.test(input)) {
    return {
      type: "date",
      value: formatIsoDate(parseIsoDate(input)),
      unitId: null,
      dimension: "date",
    };
  }

  return null;
}

function parseIsoDate(input) {
  const match = input.match(/^(\d{4})-(\d{2})-(\d{2})$/u);
  if (!match) {
    throw new Error(`Invalid ISO date: ${input}`);
  }

  const date = new Date(Date.UTC(Number(match[1]), Number(match[2]) - 1, Number(match[3])));
  if (formatIsoDate(date) !== input) {
    throw new Error(`Invalid ISO date: ${input}`);
  }
  return date;
}

function formatIsoDate(date) {
  return date.toISOString().slice(0, 10);
}
