import { EntityRef, WorldUpdate } from "../domain/world.js";

export interface IWorld {
    has_entity(entity: EntityRef): boolean;

    update(update: WorldUpdate): void;

    observe(local_ts: number, server_ts: number, ingame_ts: number): void;
}