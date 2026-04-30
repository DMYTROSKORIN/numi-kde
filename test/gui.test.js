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
