import * as lowlevel from '../lowlevel/index.js';
import * as transport from '../transport/index.js';
import { Status } from '../types/status.js';
import { Administrator } from "./administrator.js";
import { OpenSessionCallback } from "./base_module.js"

export type RootAccess = {
    open_session: OpenSessionCallback;
    close: () => Promise<unknown>;
};

export async function login(
    ip: string,
    user: string,
    password: string,
    mirroring: transport.Mirroring | undefined = undefined,
    port: number = 6842,
): Promise<[Status, RootAccess | undefined]> {

    const [status, root_session] = await lowlevel.login(
        ip,
        user,
        password,
        mirroring,
        port,
    );
    if (!status.is_ok() || !root_session) {
        return [status.wrap("failed to create root session"), undefined];
    }

    return [Status.ok(), {
        open_session: () => root_session.open_session(),
        close: () => root_session.close(),
    }];
}

export async function login_as_administrator(
    ip: string,
    port: number,
    login: string,
    password: string,
    timeout_ms: number = 500,
): Promise<[Status, Administrator | undefined]> {
    const [status, rpc] = await lowlevel.login_as_administrator(
        ip,
        port,
        login,
        password,
        timeout_ms,
    );
    if (!status.is_ok() || rpc === undefined) {
        return [status.wrap("failed to open administrator session"), undefined];
    }
    return [Status.ok(), new Administrator(rpc)];
}
