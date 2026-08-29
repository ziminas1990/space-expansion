import * as lowlevel from '../lowlevel/index.js';
import * as transport from '../transport/index.js';
import { Status } from '../types/status.js';
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
