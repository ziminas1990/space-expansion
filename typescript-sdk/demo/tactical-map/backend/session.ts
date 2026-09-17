import { login, type RootAccess } from "@spx/sdk/midlevel";
import { Status } from "@spx/sdk/types";
import { WebSocket, type RawData } from "ws";
import { World } from "../common/domain/world.js";
import {
    encode_server_message,
    parse_client_message,
    type LoginRequest,
    type ServerMessage,
} from "../common/utils/protocol.js";
import { RootCommutator } from "./controller/root_commutator.js";
import { Logger } from "./log.js";
import { ReplicatedWorld } from "./replicated_world.js";

const BROWSER_REMOTE = "browser";

export class Session {

    readonly closed: Promise<void>;

    private login_started = false;
    private shutdown_task: Promise<void> | undefined;
    private resolve_closed!: () => void;

    private root_access: RootAccess | undefined;
    private root: RootCommutator | undefined;
    private replicated: ReplicatedWorld | undefined;
    private initialize_task: Promise<Status> | undefined;

    constructor(
        private readonly socket: WebSocket,
        private readonly logger: Logger,
    ) {
        this.closed = new Promise((resolve) => {
            this.resolve_closed = resolve;
        });
        this.socket.on("message", (data) => {
            void this.on_message(data);
        });
        this.socket.on("close", () => {
            void this.shutdown("websocket closed");
        });
        this.socket.on("error", (error) => {
            this.logger.warning(`WebSocket error: ${error.message}`);
            void this.shutdown("websocket error");
        });
    }

    shutdown(reason: string): Promise<void> {
        if (this.shutdown_task) {
            return this.shutdown_task;
        }
        this.shutdown_task = this.run_shutdown(reason);
        return this.shutdown_task;
    }

    private async on_message(data: RawData): Promise<void> {
        if (this.shutdown_task) {
            return;
        }
        const parsed = parse_client_message(decode_ws_data(data));
        if (!parsed.ok) {
            this.send({ type: "login_failure", reason: parsed.error });
            return;
        }
        // Only login message is expected from the browser
        if (this.login_started) {
            this.send({ type: "login_failure", reason: "Login already processed" });
            return;
        }
        await this.handle_login(parsed.value);
    }

    private async handle_login(request: LoginRequest): Promise<void> {
        this.login_started = true;
        this.logger.info(
            `Connecting to ${request.server}:${request.port} as '${request.login}'`,
        );

        try {
            const [login_status, root_access] = await login(
                request.server,
                request.login,
                request.password,
                undefined,
                request.port,
            );
            if (this.shutdown_task) {
                if (root_access) {
                    await root_access.close();
                }
                return;
            }
            if (!root_access) {
                this.fail_login(login_status.what());
                return;
            }
            this.root_access = root_access;

            const [commutator_status, commutator] = await root_access.open_session();
            if (this.shutdown_task) {
                return;
            }
            if (!commutator || !commutator_status.is_ok()) {
                this.fail_login(
                    commutator_status.what() || "Failed to open commutator session",
                );
                return;
            }

            const world = new World(this.logger.child("world"));
            this.replicated = new ReplicatedWorld(world);
            this.root = new RootCommutator(
                commutator,
                this.replicated,
                this.logger.child("root"),
                () => this.on_root_stopped(),
            );

            this.initialize_task = this.root.initialize();
            const initialize_status = await this.initialize_task;
            if (this.shutdown_task) {
                return;
            }
            if (!initialize_status.is_ok()) {
                this.fail_login(initialize_status.what());
                return;
            }

            const verify_status = this.root.verify();
            if (this.shutdown_task) {
                return;
            }
            if (!verify_status.is_ok()) {
                this.fail_login(verify_status.what());
                return;
            }

            this.send({ type: "login_success" });
            const remote_status = this.replicated.add_remote(
                BROWSER_REMOTE,
                (message) => this.send(message),
            );
            if (!remote_status.is_ok()) {
                this.send({ type: "session_error", reason: remote_status.what() });
                await this.shutdown("failed to start replication");
            }
        } catch (error) {
            if (this.shutdown_task) {
                return;
            }
            this.fail_login(error instanceof Error ? error.message : String(error));
        }
    }

    private fail_login(reason: string): void {
        this.send({ type: "login_failure", reason });
        void this.shutdown("login failed");
    }

    private on_root_stopped(): void {
        if (this.shutdown_task) {
            return;
        }
        this.send({ type: "session_error", reason: "Game connection lost" });
        void this.shutdown("root monitoring finished");
    }

    private async run_shutdown(reason: string): Promise<void> {
        this.logger.info(`Shutting down session: ${reason}`);
        try {
            this.replicated?.remove_remote(BROWSER_REMOTE);
            if (this.initialize_task) {
                await this.initialize_task;
            }
            if (this.root && !this.root.is_stopped()) {
                const status = await this.root.stop(reason);
                if (status.is_ok()) {
                    this.root = undefined;
                }
            }
            if (this.root_access) {
                await this.root_access.close();
                this.root_access = undefined;
            }
        } catch (error) {
            this.logger.error(
                `Session shutdown error: ${
                    error instanceof Error ? error.stack ?? error.message : String(error)
                }`,
            );
        } finally {
            if (this.socket.readyState === WebSocket.OPEN
                || this.socket.readyState === WebSocket.CONNECTING)
            {
                this.socket.close();
            }
            this.resolve_closed();
        }
    }

    private send(message: ServerMessage): void {
        if (this.socket.readyState !== WebSocket.OPEN) {
            return;
        }
        this.socket.send(encode_server_message(message));
    }
}

function decode_ws_data(data: RawData): string {
    if (typeof data === "string") {
        return data;
    }
    if (Array.isArray(data)) {
        return Buffer.concat(data).toString("utf8");
    }
    if (Buffer.isBuffer(data)) {
        return data.toString("utf8");
    }
    return Buffer.from(new Uint8Array(data)).toString("utf8");
}
