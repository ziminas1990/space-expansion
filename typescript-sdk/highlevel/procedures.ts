import * as midlevel from "#sdk/midlevel/index.js"
import * as transport from "#sdk/transport/index.js"
import { Status } from "#sdk/types/status.js";
import { create_module } from "./factory.js";
import { Player } from "./player.js"

export async function login(
    ip: string,
    user: string,
    password: string,
    mirroring: transport.Mirroring | undefined = undefined,
    port: number = 6842,
): Promise<[Status, Player | undefined]> {
    const [status, root] = await midlevel.login(
        ip,
        user,
        password,
        mirroring,
        port,
    );
    if (!status.is_ok() || !root) {
        return [status, undefined];
    }
    const [open_status, commutator] = await root.open_session();
    if (!open_status.is_ok() || !commutator) {
        await root.close();
        return [open_status.wrap("failed to open root commutator"), undefined];
    }
    const player = new Player(commutator, create_module);
    const init_status = await player.init();
    if (!init_status.is_ok()) {
        await player.release();
        return [init_status.wrap("failed to initialize player"), undefined];
    }
    return [Status.ok(), player];
}
