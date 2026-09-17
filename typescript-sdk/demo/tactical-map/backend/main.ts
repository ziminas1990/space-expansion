import http from "node:http";
import { fileURLToPath } from "node:url";

import express from "express";
import { WebSocketServer } from "ws";

import { create_logger } from "./log.js";
import { Session } from "./session.js";

const DEFAULT_HOST = "127.0.0.1";
const DEFAULT_PORT = 8080;

const log = create_logger("app");

type BindAddress = {
    host: string;
    port: number;
};

function read_bind(): BindAddress {
    const host = process.env.TACTICAL_MAP_HOST ?? DEFAULT_HOST;
    const port_raw = process.env.TACTICAL_MAP_PORT ?? String(DEFAULT_PORT);
    const port = Number(port_raw);
    if (!Number.isFinite(port) || port <= 0 || port > 65535) {
        throw new Error(`Invalid TACTICAL_MAP_PORT: ${port_raw}`);
    }
    return { host, port: Math.trunc(port) };
}

function install_stop_signals(stop: AbortController): void {
    const request_stop = (signal: string) => {
        log.info(`Shutdown requested by ${signal}`);
        if (!stop.signal.aborted) {
            stop.abort();
        } else {
            log.warning(`Forced exit on second ${signal}`);
            process.exit(1);
        }
    };
    process.on("SIGINT", () => request_stop("SIGINT"));
    process.on("SIGTERM", () => request_stop("SIGTERM"));
}

function wait_signal(signal: AbortSignal): Promise<void> {
    if (signal.aborted) {
        return Promise.resolve();
    }
    return new Promise((resolve) => {
        signal.addEventListener("abort", () => resolve(), { once: true });
    });
}

function listen(server: http.Server, host: string, port: number): Promise<void> {
    return new Promise((resolve, reject) => {
        const on_error = (error: Error) => {
            reject(error);
        };
        server.once("error", on_error);
        server.listen(port, host, () => {
            server.off("error", on_error);
            resolve();
        });
    });
}

function close_http_server(server: http.Server): Promise<void> {
    return new Promise((resolve, reject) => {
        server.close((error) => {
            if (error) {
                reject(error);
            } else {
                resolve();
            }
        });
    });
}

function close_websocket_server(wss: WebSocketServer): Promise<void> {
    return new Promise((resolve, reject) => {
        wss.close((error) => {
            if (error) {
                reject(error);
            } else {
                resolve();
            }
        });
    });
}

async function run(): Promise<number> {
    let bind: BindAddress;
    try {
        bind = read_bind();
    } catch (error) {
        process.stderr.write(`${error instanceof Error ? error.message : error}\n`);
        return 2;
    }

    const frontend_dir = fileURLToPath(new URL("../frontend", import.meta.url));
    const app = express();
    app.use(express.static(frontend_dir));

    const server = http.createServer(app);
    const wss = new WebSocketServer({ server, path: "/ws" });
    const sessions = new Set<Session>();
    let next_session_id = 1;

    wss.on("connection", (socket) => {
        const session_id = next_session_id;
        next_session_id += 1;
        const session = new Session(socket, log.child(`session-${session_id}`));
        sessions.add(session);
        void session.closed.then(() => {
            sessions.delete(session);
        });
    });

    const stop = new AbortController();
    install_stop_signals(stop);

    try {
        await listen(server, bind.host, bind.port);
    } catch (error) {
        log.error(
            `Failed to listen on ${bind.host}:${bind.port}: ${
                error instanceof Error ? error.message : error
            }`,
        );
        return 1;
    }
    log.info(`Listening on http://${bind.host}:${bind.port}`);

    await wait_signal(stop.signal);
    log.info("Shutting down");

    // Close the WebSocket server first so it stops accepting, but do not wait
    // for its 'close' yet: it stays open until existing clients disconnect.
    const wss_closed = close_websocket_server(wss);
    await Promise.all(
        [...sessions].map((session) => session.shutdown("process shutdown")),
    );
    await wss_closed;
    await close_http_server(server);
    log.info("Stopped");
    return 0;
}

run().then(
    (code) => process.exit(code),
    (error) => {
        if (error instanceof Error && error.name === "AbortError") {
            process.exit(0);
        }
        log.error(error instanceof Error ? error.stack ?? error.message : String(error));
        process.exit(1);
    },
);
