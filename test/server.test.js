import assert from "node:assert/strict";
import { once } from "node:events";
import http from "node:http";
import test from "node:test";
import { spawn } from "node:child_process";

test("serves Alfred-compatible q parameter responses", async () => {
  const port = 15155;
  const child = spawn(process.execPath, ["./src/server.js"], {
    cwd: process.cwd(),
    env: { ...process.env, NUMI_KDE_PORT: String(port) },
    stdio: ["ignore", "ignore", "pipe"],
  });

  try {
    await once(child.stderr, "data");
    const body = await get(`http://127.0.0.1:${port}/?q=20%20inches%20in%20cm`);
    assert.equal(body, "50.8 cm\n");
  } finally {
    child.kill();
  }
});

function get(url) {
  return new Promise((resolve, reject) => {
    http.get(url, (response) => {
      let body = "";
      response.setEncoding("utf8");
      response.on("data", (chunk) => {
        body += chunk;
      });
      response.on("end", () => resolve(body));
    }).on("error", reject);
  });
}
