#!/usr/bin/env node
import fs from "node:fs/promises";
import http from "node:http";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { createDocumentViewModel } from "./adapter.js";

const host = process.env.NUMI_KDE_GUI_HOST ?? "127.0.0.1";
const port = Number(process.env.NUMI_KDE_GUI_PORT ?? 15156);
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../../gui");

const contentTypes = new Map([
  [".html", "text/html; charset=utf-8"],
  [".js", "text/javascript; charset=utf-8"],
  [".css", "text/css; charset=utf-8"],
]);

const server = http.createServer(async (request, response) => {
  try {
    const url = new URL(request.url ?? "/", `http://${request.headers.host ?? `${host}:${port}`}`);

    if (request.method === "POST" && url.pathname === "/api/evaluate") {
      const body = await readBody(request);
      const payload = body ? JSON.parse(body) : {};
      writeJson(response, 200, createDocumentViewModel(String(payload.source ?? "")));
      return;
    }

    if (request.method !== "GET") {
      writeText(response, 405, "Method Not Allowed\n");
      return;
    }

    const filePath = resolveStaticPath(url.pathname);
    if (!filePath) {
      writeText(response, 404, "Not Found\n");
      return;
    }

    const content = await fs.readFile(filePath);
    response.writeHead(200, { "content-type": contentTypes.get(path.extname(filePath)) ?? "application/octet-stream" });
    response.end(content);
  } catch (error) {
    writeText(response, 500, `${error instanceof Error ? error.message : String(error)}\n`);
  }
});

server.listen(port, host, () => {
  process.stderr.write(`numi-kde GUI prototype listening at http://${host}:${port}\n`);
});

function resolveStaticPath(pathname) {
  const relative = pathname === "/" ? "index.html" : pathname.slice(1);
  if (!["index.html", "app.js", "styles.css"].includes(relative)) {
    return null;
  }
  return path.join(root, relative);
}

function readBody(request) {
  return new Promise((resolve, reject) => {
    let body = "";
    request.setEncoding("utf8");
    request.on("data", (chunk) => {
      body += chunk;
    });
    request.on("end", () => resolve(body));
    request.on("error", reject);
  });
}

function writeJson(response, status, payload) {
  response.writeHead(status, { "content-type": "application/json; charset=utf-8" });
  response.end(`${JSON.stringify(payload)}\n`);
}

function writeText(response, status, text) {
  response.writeHead(status, { "content-type": "text/plain; charset=utf-8" });
  response.end(text);
}
