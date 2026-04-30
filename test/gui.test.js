import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import { once } from "node:events";
import http from "node:http";
import test from "node:test";
import { createDocumentViewModel } from "../src/gui/adapter.js";

test("creates GUI document view model from shared core", () => {
  const model = createDocumentViewModel("x = 10\nx * 2\nunknown + 1");

  assert.deepEqual(model.summary, {
    lineCount: 3,
    resultCount: 2,
    errorCount: 1,
  });
  assert.equal(model.lines[0].assignment, "x");
  assert.equal(model.lines[1].result, "20");
  assert.equal(model.lines[2].ok, false);
  assert.match(model.lines[2].diagnostics[0].message, /Unknown variable/);
});

test("creates GUI syntax highlight markup for units and natural operators", () => {
  const model = createDocumentViewModel("500 AED to USD\n5% from 100\n20 inches in cm");

  assert.match(model.lines[0].highlightedHtml, /color:#6fc4e8">AED/);
  assert.match(model.lines[0].highlightedHtml, /color:#ffd35a">to/);
  assert.match(model.lines[0].highlightedHtml, /color:#6fc4e8">USD/);
  assert.match(model.lines[1].highlightedHtml, /color:#6fc4e8">%</);
  assert.match(model.lines[1].highlightedHtml, /color:#ffd35a">from/);
  assert.match(model.lines[2].highlightedHtml, /color:#6fc4e8">inches/);
  assert.match(model.lines[2].highlightedHtml, /color:#ffd35a">in/);
  assert.match(model.lines[2].highlightedHtml, /color:#6fc4e8">cm/);
});

test("highlights defined variable names in neon green", () => {
  const model = createDocumentViewModel("A := 500\nA + 100\nB := A * 2");

  assert.match(model.lines[0].highlightedHtml, /color:#39ff14">A/);
  assert.match(model.lines[1].highlightedHtml, /color:#39ff14">A/);
  assert.match(model.lines[2].highlightedHtml, /color:#39ff14">B/);
  assert.match(model.lines[2].highlightedHtml, /color:#39ff14">A/);
});

test("/help command returns feature summary", () => {
  const model = createDocumentViewModel("/help");

  assert.equal(model.lines[0].ok, true);
  assert.ok(model.lines[0].result.length > 0);
});

test("serves GUI prototype and evaluation API", async () => {
  const port = 15157;
  const child = spawn(process.execPath, ["./src/gui/server.js"], {
    cwd: process.cwd(),
    env: { ...process.env, NUMI_KDE_GUI_PORT: String(port) },
    stdio: ["ignore", "ignore", "pipe"],
  });

  try {
    await once(child.stderr, "data");
    const html = await request({
      method: "GET",
      url: `http://127.0.0.1:${port}/`,
    });
    assert.match(html, /numi-kde GUI Prototype/);

    const body = await request({
      method: "POST",
      url: `http://127.0.0.1:${port}/api/evaluate`,
      body: JSON.stringify({ source: "20 inches in cm" }),
    });
    const model = JSON.parse(body);
    assert.equal(model.lines[0].result, "50.8 cm");
  } finally {
    child.kill();
  }
});

function request({ method, url, body = null }) {
  return new Promise((resolve, reject) => {
    const requestOptions = new URL(url);
    requestOptions.method = method;
    requestOptions.headers = body ? { "content-type": "application/json" } : {};

    const req = http.request(requestOptions, (response) => {
      let responseBody = "";
      response.setEncoding("utf8");
      response.on("data", (chunk) => {
        responseBody += chunk;
      });
      response.on("end", () => resolve(responseBody));
    });
    req.on("error", reject);
    if (body) {
      req.write(body);
    }
    req.end();
  });
}
