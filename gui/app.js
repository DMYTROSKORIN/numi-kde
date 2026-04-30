const source = document.querySelector("#source");
const results = document.querySelector("#results");
const status = document.querySelector("#status");
const copyResults = document.querySelector("#copy-results");

let lastModel = null;
let pending = null;

source.addEventListener("input", () => scheduleEvaluate());
source.addEventListener("scroll", () => {
  results.scrollTop = source.scrollTop;
});

copyResults.addEventListener("click", async () => {
  const text = (lastModel?.lines ?? [])
    .map((line) => line.result)
    .filter(Boolean)
    .join("\n");
  await navigator.clipboard.writeText(text);
  status.textContent = "Results copied";
});

scheduleEvaluate();

function scheduleEvaluate() {
  clearTimeout(pending);
  pending = setTimeout(evaluateSource, 80);
}

async function evaluateSource() {
  const response = await fetch("/api/evaluate", {
    method: "POST",
    headers: { "content-type": "application/json" },
    body: JSON.stringify({ source: source.value }),
  });
  lastModel = await response.json();
  render(lastModel);
}

function render(model) {
  results.replaceChildren(...model.lines.map(renderLine));
  status.textContent = `${model.summary.resultCount} results, ${model.summary.errorCount} errors`;
}

function renderLine(line) {
  const row = document.createElement("div");
  row.className = `result-line${line.ok ? "" : " error"}`;
  row.title = line.diagnostics.map((diagnostic) => diagnostic.message).join("\n");

  const number = document.createElement("span");
  number.className = "line-number";
  number.textContent = line.number;

  const value = document.createElement("strong");
  value.textContent = line.ok ? line.result : line.diagnostics[0]?.message ?? "";

  row.append(number, value);
  return row;
}
