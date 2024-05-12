import * as midlevel from "../midlevel/index.js"
import * as transport from "../transport/index.js"
import { Status } from "../types/status.js";
import { Player } from "./player.js"
import { Commutator } from "./commutator.js";

export async function login(
    ip: string,
    user: string,
    password: string,
    mirroring: transport.Mirroring | undefined = undefined
): Promise<[Status, Player | undefined]> {
    const [status, midlevel_comm] = await midlevel.login(ip, user, password, mirroring);
    if (!status.is_ok() || !midlevel_comm) {
        return [status, undefined];
    }
    const player = new Player(new Commutator(midlevel_comm));
    return [Status.ok(), player];
}