import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import path from "node:path";

import { login } from "../../highlevel/index.js";
import { create_logger, set_log_level } from "./log.js";
import { TacticalCore } from "./tactical_core.js";

const DEFAULTS = {
    server: "127.0.0.1",
    port: 6842,
    login: "Olenoid",
    password: "admin",
};

type Credentials = {
    server: string;
    port: number;
    login: string;
    password: string;
};

type Args = {
    config?: string;
    server?: string;
    port?: number;
    login?: string;
    password?: string;
    log_level: string;
};

const log = create_logger("harvester");

function print_help(): void {
    process.stdout.write(
        "Headless harvester: mine asteroids and build miner ships.\n\n"
        + "Options:\n"
        + "  --config PATH       JSON file with server, port, login, and password\n"
        + "  --server ADDRESS    Server IP address\n"
        + "  --port PORT         Login port\n"
        + "  --login LOGIN       Player login\n"
        + "  --password PASSWORD Player password\n"
        + "  --log-level LEVEL   Logging level (DEBUG, INFO, WARNING, ERROR)\n"
        + "  --help              Show this help\n",
    );
}

export function parse_args(argv: string[] = process.argv.slice(2)): Args {
    const args: Args = { log_level: "INFO" };
    for (let i = 0; i < argv.length; i += 1) {
        const arg = argv[i];
        const next = argv[i + 1];
        switch (arg) {
            case "--help":
            case "-h":
                print_help();
                process.exit(0);
            case "--config":
                if (next === undefined) {
                    throw new Error("--config requires a path");
                }
                args.config = next;
                i += 1;
                break;
            case "--server":
                if (next === undefined) {
                    throw new Error("--server requires an address");
                }
                args.server = next;
                i += 1;
                break;
            case "--port":
                if (next === undefined) {
                    throw new Error("--port requires a number");
                }
                args.port = Number(next);
                i += 1;
                break;
            case "--login":
                if (next === undefined) {
                    throw new Error("--login requires a value");
                }
                args.login = next;
                i += 1;
                break;
            case "--password":
                if (next === undefined) {
                    throw new Error("--password requires a value");
                }
                args.password = next;
                i += 1;
                break;
            case "--log-level":
                if (next === undefined) {
                    throw new Error("--log-level requires a value");
                }
                args.log_level = next;
                i += 1;
                break;
            default:
                throw new Error(`Unknown argument: ${arg}`);
        }
    }
    return args;
}

export function load_credentials(args: Args): Credentials {
    const credentials: Credentials = { ...DEFAULTS };
    if (args.config !== undefined) {
        let raw: unknown;
        try {
            raw = JSON.parse(readFileSync(args.config, "utf8"));
        } catch (error) {
            if (error instanceof SyntaxError) {
                throw new Error(`Invalid JSON in '${args.config}': ${error.message}`);
            }
            throw new Error(`Failed to read config '${args.config}': ${error}`);
        }
        if (raw === null || typeof raw !== "object" || Array.isArray(raw)) {
            throw new Error(`Config '${args.config}' must be a JSON object`);
        }
        const object = raw as Record<string, unknown>;
        if (typeof object.server === "string") {
            credentials.server = object.server;
        }
        if (object.port !== undefined) {
            credentials.port = Number(object.port);
        }
        if (typeof object.login === "string") {
            credentials.login = object.login;
        }
        if (typeof object.password === "string") {
            credentials.password = object.password;
        }
    }
    if (args.server !== undefined) {
        credentials.server = args.server;
    }
    if (args.port !== undefined) {
        credentials.port = args.port;
    }
    if (args.login !== undefined) {
        credentials.login = args.login;
    }
    if (args.password !== undefined) {
        credentials.password = args.password;
    }
    if (!Number.isFinite(credentials.port)) {
        throw new Error(`Invalid port: ${credentials.port}`);
    }
    credentials.port = Math.trunc(credentials.port);
    return credentials;
}

function install_stop_signals(stop: AbortController): void {
    const request_stop = () => {
        if (!stop.signal.aborted) {
            stop.abort();
        }
    };
    process.on("SIGINT", request_stop);
    process.on("SIGTERM", request_stop);
}

async function async_main(credentials: Credentials): Promise<number> {
    const stop = new AbortController();
    install_stop_signals(stop);

    let tactical_core: TacticalCore | undefined;

    const operate = async (): Promise<number> => {
        log.info(
            `Connecting to ${credentials.server}:${credentials.port} `
            + `as '${credentials.login}'`,
        );
        const [status, player] = await login(
            credentials.server,
            credentials.login,
            credentials.password,
            undefined,
            credentials.port,
        );
        if (!player) {
            log.error(`Failed to login: ${status.what()}`);
            return 1;
        }

        tactical_core = new TacticalCore(player);
        if (!await tactical_core.initialize()) {
            log.error("Failed to initialize tactical core!");
            await tactical_core.stop();
            tactical_core = undefined;
            return 1;
        }

        log.info("Harvester is running. Press Ctrl+C to stop.");
        const running = tactical_core.run();
        await Promise.race([
            running,
            wait_signal(stop.signal),
        ]);
        if (!stop.signal.aborted) {
            log.error("Tactical core stopped unexpectedly");
            return 1;
        }
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
    } finally {
        if (tactical_core !== undefined) {
            await tactical_core.stop();
        }
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

export function run(argv?: string[]): void {
    let args: Args;
    try {
        args = parse_args(argv);
    } catch (error) {
        process.stderr.write(`${error instanceof Error ? error.message : error}\n`);
        process.exit(2);
    }
    set_log_level(args.log_level);
    let credentials: Credentials;
    try {
        credentials = load_credentials(args);
    } catch (error) {
        process.stderr.write(`${error instanceof Error ? error.message : error}\n`);
        process.exit(2);
    }
    async_main(credentials).then(
        (code) => process.exit(code),
        (error) => {
            if (error instanceof Error && error.name === "AbortError") {
                process.exit(0);
            }
            log.error(error instanceof Error ? error.stack ?? error.message : String(error));
            process.exit(1);
        },
    );
}

function is_direct_run(): boolean {
    const entry = process.argv[1];
    if (!entry) {
        return false;
    }
    return path.resolve(entry) === fileURLToPath(import.meta.url);
}

if (is_direct_run()) {
    run();
}
