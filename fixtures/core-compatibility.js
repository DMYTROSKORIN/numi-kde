export const expressionFixtures = [
  ["arithmetic precedence", "2 + 3 * 4", "14"],
  ["parenthesized arithmetic", "(2 + 3) * 4", "20"],
  ["right associative powers", "2^3^2", "512"],
  ["unary minus", "-2^2", "-4"],
  ["implicit parentheses multiplication", "2(3 + 4)", "14"],
  ["implicit constant multiplication", "2pi", "6.28318530718"],
  ["thousands separators", "1,234.5 + 5.5", "1240"],
  ["decimal comma", "1,5 + 2", "3.5"],
  ["constants", "round(pi)", "3"],
  ["functions", "sqrt(81)", "9"],
  ["aggregate functions", "avg(2;4;6)", "4"],
  ["percentage of", "20% of 50", "10"],
  ["percentage addition", "200 + 10%", "220"],
  ["percentage subtraction", "200 - 10%", "180"],
  ["percentage addition after expression", "2pi + 200 + 10%", "226.911503837898"],
  ["percentage ratio", "10 as % of 200", "5%"],
  ["length conversion", "20 inches in cm", "50.8 cm"],
  ["css unit conversion", "32px in rem", "2 rem"],
  ["time conversion", "2 hours in minutes", "120 min"],
  ["mass conversion", "2 lb in kg", "0.90718474 kg"],
  ["data conversion", "1 MiB in bytes", "1048576 byte"],
  ["bit byte case-sensitive conversion", "8 b in B", "1 byte"],
  ["temperature conversion", "32 F in C", "0 C"],
  ["iso date literal", "2026-04-30", "2026-04-30"],
  ["iso date plus days", "2026-04-30 + 2 days", "2026-05-02"],
  ["iso date plus weeks", "2026-04-30 + 2 weeks", "2026-05-14"],
  ["iso date minus week", "2026-04-30 - 1 week", "2026-04-23"],
  ["iso date difference", "2026-05-15 - 2026-04-30", "15 days"],
];

export const documentFixtures = [
  {
    name: "multi-line variables and summary tokens",
    source: "x = 10\ny = x * 2\nprev + sum + avg",
    results: ["10", "20", "65"],
  },
  {
    name: "blank and comment lines",
    source: "x = 10\n\n# ignored\nx * 3 // inline note",
    results: ["10", "", "", "30"],
  },
  {
    name: "colon assignment",
    source: "subtotal := 25\nsubtotal + 5",
    results: ["25", "30"],
  },
];

export const formattingFixtures = [
  {
    name: "custom precision",
    source: "1 / 3",
    options: { precision: 4 },
    expected: "0.3333",
  },
  {
    name: "fixed fraction digits preserve trailing zeros",
    source: "2",
    options: { minimumFractionDigits: 2, maximumFractionDigits: 2 },
    expected: "2.00",
  },
  {
    name: "grouping separators",
    source: "1234567.89",
    options: { useGrouping: true },
    expected: "1,234,567.89",
  },
  {
    name: "scientific notation threshold",
    source: "1000000000000",
    options: { scientificNotationThreshold: 1000000 },
    expected: "1e+12",
  },
  {
    name: "unit formatting inherits numeric options",
    source: "2 lb in kg",
    options: { precision: 3 },
    expected: "0.907 kg",
  },
];
