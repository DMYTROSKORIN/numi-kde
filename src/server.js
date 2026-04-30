#!/usr/bin/env node
import http from "node:http";
import { evaluate } from "./core/engine.js";

const host = process.env.NUMI_KDE_HOST ?? "127.0.0.1";
const port = Number(process.env.NUMI_KDE_PORT ?? 15055);

const server = http.createServer((request, response) => {
  const url = new URL(request.url ?? "/", `http://${request.headers.host ?? `${host}:${port}`}`);

  if (request.method !== "GET") {
    response.writeHead(405, { "content-type": "text/plain; charset=utf-8" });
    response.end("Method Not Allowed\n");
    return;
  }

  if (url.pathname !== "/") {
    response.writeHead(404, { "content-type": "text/plain; charset=utf-8" });
    response.end("Not Found\n");
    return;
  }

  const query = url.searchParams.get("q") ?? "";
  const result = evaluate(query);

  if (!result.ok) {
    response.writeHead(422, { "content-type": "text/plain; charset=utf-8" });
    response.end(`${result.error}\n`);
    return;
  }

  response.writeHead(200, { "content-type": "text/plain; charset=utf-8" });
  response.end(`${result.formatted}\n`);
});

server.listen(port, host, () => {
  process.stderr.write(`numi-kde server listening at http://${host}:${port}\n`);
});
