import { Status } from "@spx/sdk/types";
import { EntityRef, World, WorldUpdate } from "../common/domain/world.js";
import { ServerMessage, pack_world_update } from "../common/utils/protocol.js";
import { IWorld } from "./controller/interfaces.js";

export type MessageSink = (message: ServerMessage) => void;

export class ReplicatedWorld implements IWorld {

    private readonly remotes = new Map<string, MessageSink>();
    private latest_clock: { server_ts: number; ingame_ts: number } | undefined;

    constructor(
        private readonly world: World,
    ) {}

    has_entity(entity: EntityRef): boolean {
        return this.world.has_entity(entity);
    }

    update(update: WorldUpdate): void {
        this.world.update(update);
        this.broadcast({
            type: "world_update",
            update: pack_world_update(update),
        });
    }

    observe(local_ts: number, server_ts: number, ingame_ts: number): void {
        this.world.observe(local_ts, server_ts, ingame_ts);
        this.latest_clock = { server_ts, ingame_ts };
        this.broadcast({
            type: "clock",
            server_ts,
            ingame_ts,
        });
    }

    // Register a named replica, then send it a packed snapshot and the latest
    // clock observation (if any). Must stay synchronous so later updates
    // cannot overtake the snapshot on the wire.
    add_remote(name: string, send: MessageSink): Status {
        if (this.remotes.has(name)) {
            return Status.fail(`Remote '${name}' already exists`);
        }
        send({
            type: "snapshot",
            world: this.world.pack(),
        });
        if (this.latest_clock !== undefined) {
            send({
                type: "clock",
                server_ts: this.latest_clock.server_ts,
                ingame_ts: this.latest_clock.ingame_ts,
            });
        }
        this.remotes.set(name, send);
        return Status.ok();
    }

    remove_remote(name: string): void {
        this.remotes.delete(name);
    }

    private broadcast(message: ServerMessage): void {
        for (const send of this.remotes.values()) {
            send(message);
        }
    }

}
