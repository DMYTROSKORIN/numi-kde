#!/usr/bin/env node
import { evaluateDocument } from "./core/engine.js";

const input = process.argv.slice(2).join(" ");

if (!input) {
  process.stderr.write("Usage: numi-kde \"20 inches in cm\"\n");
  process.exit(2);
}

const source = input.replaceAll("\\n", "\n");
const results = evaluateDocument(source);
const last = [...results].reverse().find((line) => line.ok && line.result !== null);
const firstError = results.find((line) => !line.ok);

if (firstError) {
  process.stderr.write(`Line ${firstError.line}: ${firstError.error}\n`);
  process.exit(1);
}

process.stdout.write(`${last?.formatted ?? ""}\n`);
