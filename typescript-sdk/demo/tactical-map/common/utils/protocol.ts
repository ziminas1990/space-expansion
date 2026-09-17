import type { WorldPacked } from "../domain/world.js";
import type { WorldUpdatePacked } from "./world_update.js";

export type { WorldUpdatePacked } from "./world_update.js";
export { pack_world_update, unpack_world_update } from "./world_update.js";

// Browser-safe WebSocket JSON contract.
// Clock messages carry only server_ts and ingame_ts; the receiver supplies local time.
// Never log or echo login passwords.

export type LoginRequest = {
    type: "login";
    server: string;
    port: number;
    login: string;
    password: string;
};

export type LoginSuccess = {
    type: "login_success";
};

export type LoginFailure = {
    type: "login_failure";
    reason: string;
};

export type WorldSnapshot = {
    type: "snapshot";
    world: WorldPacked;
};

export type WorldUpdateMessage = {
    type: "world_update";
    update: WorldUpdatePacked;
};

export type ClockObservation = {
    type: "clock";
    server_ts: number;
    ingame_ts: number;
};

export type SessionError = {
    type: "session_error";
    reason: string;
};

export type ClientMessage = LoginRequest;

export type ServerMessage =
    | LoginSuccess
    | LoginFailure
    | WorldSnapshot
    | WorldUpdateMessage
    | ClockObservation
    | SessionError;

export type ParseSuccess<T> = { ok: true; value: T };
export type ParseFailure = { ok: false; error: string };
export type ParseResult<T> = ParseSuccess<T> | ParseFailure;

export function encode_client_message(message: ClientMessage): string {
    return JSON.stringify(message);
}

export function encode_server_message(message: ServerMessage): string {
    return JSON.stringify(message);
}

export function parse_client_message(raw: unknown): ParseResult<ClientMessage> {
    const decoded = decode_json(raw);
    if (!decoded.ok) {
        return decoded;
    }
    if (!is_record(decoded.value)) {
        return { ok: false, error: "Invalid message" };
    }
    const type = decoded.value.type;
    if (type !== "login") {
        return { ok: false, error: "Invalid message" };
    }
    return parse_login_request(decoded.value);
}

export function parse_server_message(raw: unknown): ParseResult<ServerMessage> {
    const decoded = decode_json(raw);
    if (!decoded.ok) {
        return decoded;
    }
    if (!is_record(decoded.value)) {
        return { ok: false, error: "Invalid message" };
    }
    const type = decoded.value.type;
    switch (type) {
        case "login_success":
            return { ok: true, value: { type: "login_success" } };
        case "login_failure":
            if (typeof decoded.value.reason !== "string") {
                return { ok: false, error: "Invalid message" };
            }
            return {
                ok: true,
                value: { type: "login_failure", reason: decoded.value.reason },
            };
        case "snapshot":
            return parse_snapshot(decoded.value);
        case "world_update":
            return parse_world_update_message(decoded.value);
        case "clock":
            return parse_clock(decoded.value);
        case "session_error":
            if (typeof decoded.value.reason !== "string") {
                return { ok: false, error: "Invalid message" };
            }
            return {
                ok: true,
                value: { type: "session_error", reason: decoded.value.reason },
            };
        default:
            return { ok: false, error: "Invalid message" };
    }
}

function decode_json(raw: unknown): ParseResult<unknown> {
    if (typeof raw === "string") {
        try {
            return { ok: true, value: JSON.parse(raw) };
        } catch {
            return { ok: false, error: "Invalid JSON" };
        }
    }
    return { ok: true, value: raw };
}

function parse_login_request(value: Record<string, unknown>): ParseResult<LoginRequest> {
    if (typeof value.server !== "string") {
        return { ok: false, error: "Invalid login request" };
    }
    if (typeof value.port !== "number" || !Number.isFinite(value.port)) {
        return { ok: false, error: "Invalid login request" };
    }
    if (typeof value.login !== "string") {
        return { ok: false, error: "Invalid login request" };
    }
    if (typeof value.password !== "string") {
        return { ok: false, error: "Invalid login request" };
    }
    return {
        ok: true,
        value: {
            type: "login",
            server: value.server,
            port: Math.trunc(value.port),
            login: value.login,
            password: value.password,
        },
    };
}

function parse_snapshot(value: Record<string, unknown>): ParseResult<WorldSnapshot> {
    if (!Array.isArray(value.world)) {
        return { ok: false, error: "Invalid message" };
    }
    return {
        ok: true,
        value: { type: "snapshot", world: value.world as unknown as WorldPacked },
    };
}

function parse_world_update_message(
    value: Record<string, unknown>,
): ParseResult<WorldUpdateMessage> {
    if (!is_record(value.update) || typeof value.update.type !== "string") {
        return { ok: false, error: "Invalid message" };
    }
    return {
        ok: true,
        value: {
            type: "world_update",
            update: value.update as WorldUpdatePacked,
        },
    };
}

function parse_clock(value: Record<string, unknown>): ParseResult<ClockObservation> {
    if (typeof value.server_ts !== "number" || !Number.isFinite(value.server_ts)) {
        return { ok: false, error: "Invalid message" };
    }
    if (typeof value.ingame_ts !== "number" || !Number.isFinite(value.ingame_ts)) {
        return { ok: false, error: "Invalid message" };
    }
    return {
        ok: true,
        value: {
            type: "clock",
            server_ts: value.server_ts,
            ingame_ts: value.ingame_ts,
        },
    };
}

function is_record(value: unknown): value is Record<string, unknown> {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}
