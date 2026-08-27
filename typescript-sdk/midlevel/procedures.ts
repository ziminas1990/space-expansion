import * as lowlevel from '../lowlevel/index.js';
import * as transport from '../transport/index.js';
import { Status } from '../types/status.js';
import { Commutator } from "./commutator.js"

export async function login(
    ip: string,
    user: string,
    password: string,
    mirroring: transport.Mirroring | undefined = undefined
): Promise<[Status, Commutator | undefined]> {

    const [status, root_session] = await lowlevel.login(ip, user, password, mirroring);
    if (!status.is_ok() || !root_session) {
        return [status.wrap("failed to create root session"), undefined];
    }

    const commutator = new Commutator(() => root_session.open_session());
    return [Status.ok(), commutator];
}