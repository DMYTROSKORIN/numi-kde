#!/usr/bin/env node
import { createDocumentViewModel } from "./adapter.js";

let source = "";
process.stdin.setEncoding("utf8");
process.stdin.on("data", (chunk) => {
  source += chunk;
});
process.stdin.on("end", () => {
  process.stdout.write(`${JSON.stringify(createDocumentViewModel(source))}\n`);
});
