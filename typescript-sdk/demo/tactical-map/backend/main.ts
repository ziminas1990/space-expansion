import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";

import { login } from "@spx/sdk/midlevel";
import { create_logger } from "./log.js";
import { RootCommutator } from "./controller/root_commutator.js";
import { World } from "../common/domain/world.js";

const CONFIG_PATH = fileURLToPath(new URL("../../config.json", import.meta.url));

type Credentials = {
    server: string;
    port: number;
    login: string;
    password: string;
};

const log = create_logger("tactical-map");

function load_credentials(): Credentials {
    let raw: unknown;
    try {
        raw = JSON.parse(readFileSync(CONFIG_PATH, "utf8"));
    } catch (error) {
        if (error instanceof SyntaxError) {
            throw new Error(`Invalid JSON in '${CONFIG_PATH}': ${error.message}`);
        }
        throw new Error(`Failed to read config '${CONFIG_PATH}': ${error}`);
    }
    if (raw === null || typeof raw !== "object" || Array.isArray(raw)) {
        throw new Error(`Config '${CONFIG_PATH}' must be a JSON object`);
    }
    const object = raw as Record<string, unknown>;
    if (typeof object.server !== "string") {
        throw new Error(`Config '${CONFIG_PATH}' must have a string 'server'`);
    }
    if (typeof object.login !== "string") {
        throw new Error(`Config '${CONFIG_PATH}' must have a string 'login'`);
    }
    if (typeof object.password !== "string") {
        throw new Error(`Config '${CONFIG_PATH}' must have a string 'password'`);
    }
    const port = Number(object.port);
    if (!Number.isFinite(port)) {
        throw new Error(`Invalid port: ${object.port}`);
    }
    return {
        server: object.server,
        port: Math.trunc(port),
        login: object.login,
        password: object.password,
    };
}

function install_stop_signals(stop: AbortController): void {
    const request_stop = (signal: string) => {
        log.info(`Shutdown requested by ${signal}`);
        if (!stop.signal.aborted) {
            stop.abort();
        }
    };
    process.on("SIGINT", () => request_stop("SIGINT"));
    process.on("SIGTERM", () => request_stop("SIGTERM"));
}

async function run(): Promise<number> {
    let credentials: Credentials;
    try {
        credentials = load_credentials();
    } catch (error) {
        process.stderr.write(`${error instanceof Error ? error.message : error}\n`);
        return 2;
    }

    const stop = new AbortController();
    install_stop_signals(stop);

    const operate = async (): Promise<number> => {

        // Login to server
        log.info(
            `Connecting to ${credentials.server}:${credentials.port} `
            + `as '${credentials.login}'`,
        );
        const [status, root_access] = await login(
            credentials.server,
            credentials.login,
            credentials.password,
            undefined,
            credentials.port,
        );
        if (!root_access) {
            log.error(`Failed to login: ${status.what()}`);
            return 1;
        }

        const [commutator_status, commutator] = await root_access.open_session();
        if (!commutator || !commutator_status.is_ok()) {
            log.error(`Failed to open commutator: ${commutator_status.what()}`);
            return 1;
        }

        const journal = create_logger("app");

        // Create world and root commutator
        const world = new World(journal.child("world"));

        const root_commutator = new RootCommutator(commutator, world, log);
        const initialize_status = await root_commutator.initialize();
        if (!initialize_status.is_ok()) {
            log.error(`Failed to initialize root commutator: ${initialize_status.what()}`);
            return 1;
        }

        const verify_status = root_commutator.verify();
        if (!verify_status.is_ok()) {
            log.error(`Root commutator verification failed: ${verify_status.what()}`);
            await root_commutator.stop("verification failed");
            return 1;
        }

        while (!root_commutator.is_stopped()) {
            await new Promise(resolve => setTimeout(resolve, 100));
            if (stop.signal.aborted) {
                journal.info("Stopping controller");
                await root_commutator.stop("shutdown requested");
            }
        }
        log.info("Applicationstopped");
        return 0;
    };

    try {
        const operate_task = operate();
        const result = await Promise.race([
            operate_task.then((code) => ({ kind: "done" as const, code })),
            wait_signal(stop.signal).then(() => ({ kind: "stop" as const })),
        ]);
        if (result.kind === "done") {
            return result.code;
        }
        log.info("Shutdown requested");
        return 0;
    }
    finally {
        stop.abort();
    }
}

function wait_signal(signal: AbortSignal): Promise<void> {
    if (signal.aborted) {
        return Promise.resolve();
    }
    return new Promise((resolve) => {
        signal.addEventListener("abort", () => resolve(), { once: true });
    });
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
