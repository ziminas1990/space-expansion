import { local_now_us } from "@spx/sdk/utils";
import { World } from "../common/domain/world.js";
import {
    create_console_logger,
    crate_formatted_logger,
    type ILogger,
} from "../common/logger.js";
import {
    encode_client_message,
    parse_server_message,
    type LoginRequest,
    type ServerMessage,
} from "../common/utils/protocol.js";
import { unpack_world_update } from "../common/utils/world_update.js";

export type LoginCredentials = {
    server: string;
    port: number;
    login: string;
    password: string;
};

export type ClientStatus =
    | { type: "disconnected"; reason?: string }
    | { type: "connecting" }
    | { type: "login_error"; reason: string }
    | { type: "connected" };

export type WorldClientOptions = {
    logger?: ILogger;
    websocket_url?: string;
};

export class WorldClient {

    readonly subscribe = (on_store_change: () => void): (() => void) => {
        this.listeners.add(on_store_change);
        return () => {
            this.listeners.delete(on_store_change);
        };
    };

    readonly get_version = (): number => this.version;

    private readonly listeners = new Set<() => void>();
    private readonly logger: ILogger;
    private readonly websocket_url: string | undefined;

    private version = 0;
    private status: ClientStatus = { type: "disconnected" };
    private world: World | undefined;
    private followed_ship_id: string | undefined;
    private socket: WebSocket | undefined;

    constructor(options: WorldClientOptions = {}) {
        this.logger = options.logger ?? crate_formatted_logger(
            "world-client",
            create_console_logger(),
        );
        this.websocket_url = options.websocket_url;
    }

    get_status(): ClientStatus {
        return this.status;
    }

    get_world(): World | undefined {
        return this.world;
    }

    get_followed_ship_id(): string | undefined {
        return this.followed_ship_id;
    }

    set_followed_ship_id(id: string | undefined): void {
        if (id !== undefined && this.world?.get_player_ship(id) === undefined) {
            return;
        }
        if (this.followed_ship_id === id) {
            return;
        }
        this.followed_ship_id = id;
        this.notify();
    }

    connect(credentials: LoginCredentials): void {
        this.close_socket();
        this.world = undefined;
        this.followed_ship_id = undefined;
        this.status = { type: "connecting" };
        this.notify();

        const login_request: LoginRequest = {
            type: "login",
            server: credentials.server,
            port: credentials.port,
            login: credentials.login,
            password: credentials.password,
        };
        const socket = new WebSocket(this.websocket_url ?? default_websocket_url());
        this.socket = socket;

        socket.onopen = () => {
            if (this.socket !== socket) {
                return;
            }
            socket.send(encode_client_message(login_request));
        };
        socket.onmessage = (event: MessageEvent) => {
            if (this.socket !== socket) {
                return;
            }
            this.handle_message(event.data);
        };
        socket.onerror = () => {
            if (this.socket !== socket) {
                return;
            }
            this.logger.warning("WebSocket error");
        };
        socket.onclose = () => {
            if (this.socket !== socket) {
                return;
            }
            this.socket = undefined;
            this.drop_world();
            if (this.status.type === "login_error" || this.status.type === "disconnected") {
                this.notify();
                return;
            }
            this.status = { type: "disconnected", reason: "Disconnected" };
            this.notify();
        };
    }

    disconnect(): void {
        if (this.status.type !== "disconnected") {
            this.status = { type: "disconnected" };
        }
        this.drop_world();
        this.close_socket();
        this.notify();
    }

    // Apply a parsed or JSON-encoded server message. Used by the WebSocket
    // path and by tests that feed snapshot, update, and clock traffic directly.
    handle_message(raw: unknown): void {
        const parsed = parse_server_message(raw);
        if (!parsed.ok) {
            this.logger.warning("Invalid server message");
            return;
        }
        this.apply_message(parsed.value);
    }

    private apply_message(message: ServerMessage): void {
        switch (message.type) {
            case "login_success":
                this.status = { type: "connected" };
                this.notify();
                return;
            case "login_failure":
                this.status = { type: "login_error", reason: message.reason };
                this.drop_world();
                this.close_socket();
                this.notify();
                return;
            case "snapshot":
                this.world = World.unpack(message.world, this.logger.child("world"));
                this.clear_follow_if_missing();
                this.notify();
                return;
            case "world_update":
                if (this.world === undefined) {
                    return;
                }
                this.world.update(unpack_world_update(message.update));
                this.clear_follow_if_missing();
                this.notify();
                return;
            case "clock":
                if (this.world === undefined) {
                    return;
                }
                this.world.observe(local_now_us(), message.server_ts, message.ingame_ts);
                this.notify();
                return;
            case "session_error":
                this.status = { type: "disconnected", reason: message.reason };
                this.drop_world();
                this.close_socket();
                this.notify();
                return;
        }
    }

    private drop_world(): void {
        this.world = undefined;
        this.followed_ship_id = undefined;
    }

    private clear_follow_if_missing(): void {
        if (this.followed_ship_id === undefined) {
            return;
        }
        if (this.world?.get_player_ship(this.followed_ship_id) === undefined) {
            this.followed_ship_id = undefined;
        }
    }

    private close_socket(): void {
        const socket = this.socket;
        if (socket === undefined) {
            return;
        }
        this.socket = undefined;
        socket.onopen = null;
        socket.onmessage = null;
        socket.onerror = null;
        socket.onclose = null;
        if (socket.readyState === WebSocket.OPEN
            || socket.readyState === WebSocket.CONNECTING)
        {
            socket.close();
        }
    }

    private notify(): void {
        this.version += 1;
        for (const listener of this.listeners) {
            listener();
        }
    }

}

function default_websocket_url(): string {
    const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
    return `${protocol}//${window.location.host}/ws`;
}
