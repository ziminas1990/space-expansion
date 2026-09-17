import { Asteroid, AsteroidPacked, AsteroidUpdate } from "../domain/asteroid.js";
import { PlayerShip, PlayerShipPacked } from "../domain/player_ship.js";
import { pack_position, unpack_position } from "../domain/position.js";
import { Ship, ShipPacked, ShipUpdate } from "../domain/ship.js";
import { EntityRef, WorldUpdate } from "../domain/world.js";

export type PositionPacked = ReturnType<typeof pack_position>;

export type AsteroidUpdatePacked = {
    position?: PositionPacked;
    radius?: number;
};

export type ShipUpdatePacked = {
    position?: PositionPacked;
};

export type WorldUpdatePacked =
    | { type: "add_asteroid"; asteroid: AsteroidPacked }
    | { type: "add_ship"; ship: ShipPacked }
    | { type: "add_player_ship"; ship: PlayerShipPacked }
    | { type: "asteroid_update"; asteroid_id: string; update: AsteroidUpdatePacked }
    | { type: "ship_update"; ship_id: string; update: ShipUpdatePacked }
    | { type: "player_ship_update"; ship_id: string; update: ShipUpdatePacked }
    | { type: "remove_entity"; entity: EntityRef };

export function pack_world_update(update: WorldUpdate): WorldUpdatePacked {
    switch (update.type) {
        case "add_asteroid":
            return {
                type: "add_asteroid",
                asteroid: update.asteroid.pack(),
            };
        case "add_ship":
            return {
                type: "add_ship",
                ship: update.ship.pack(),
            };
        case "add_player_ship":
            return {
                type: "add_player_ship",
                ship: update.ship.pack(),
            };
        case "asteroid_update":
            return {
                type: "asteroid_update",
                asteroid_id: update.asteroid_id,
                update: pack_asteroid_update(update.update),
            };
        case "ship_update":
            return {
                type: "ship_update",
                ship_id: update.ship_id,
                update: pack_ship_update(update.update),
            };
        case "player_ship_update":
            return {
                type: "player_ship_update",
                ship_id: update.ship_id,
                update: pack_ship_update(update.update),
            };
        case "remove_entity":
            return {
                type: "remove_entity",
                entity: {
                    kind: update.entity.kind,
                    id: update.entity.id,
                },
            };
        default: {
            const unexpected: never = update;
            throw new Error(`Unknown update type: ${(unexpected as WorldUpdate).type}`);
        }
    }
}

export function unpack_world_update(packed: WorldUpdatePacked): WorldUpdate {
    switch (packed.type) {
        case "add_asteroid":
            return {
                type: "add_asteroid",
                asteroid: Asteroid.unpack(packed.asteroid),
            };
        case "add_ship":
            return {
                type: "add_ship",
                ship: Ship.unpack(packed.ship),
            };
        case "add_player_ship":
            return {
                type: "add_player_ship",
                ship: PlayerShip.unpack(packed.ship),
            };
        case "asteroid_update":
            return {
                type: "asteroid_update",
                asteroid_id: packed.asteroid_id,
                update: unpack_asteroid_update(packed.update),
            };
        case "ship_update":
            return {
                type: "ship_update",
                ship_id: packed.ship_id,
                update: unpack_ship_update(packed.update),
            };
        case "player_ship_update":
            return {
                type: "player_ship_update",
                ship_id: packed.ship_id,
                update: unpack_ship_update(packed.update),
            };
        case "remove_entity":
            return {
                type: "remove_entity",
                entity: {
                    kind: packed.entity.kind,
                    id: packed.entity.id,
                },
            };
        default: {
            const unexpected: never = packed;
            throw new Error(`Unknown update type: ${(unexpected as WorldUpdatePacked).type}`);
        }
    }
}

function pack_asteroid_update(update: AsteroidUpdate): AsteroidUpdatePacked {
    const packed: AsteroidUpdatePacked = {};
    if (update.position !== undefined) {
        packed.position = pack_position(update.position);
    }
    if (update.radius !== undefined) {
        packed.radius = update.radius;
    }
    return packed;
}

function unpack_asteroid_update(packed: AsteroidUpdatePacked): AsteroidUpdate {
    const update: AsteroidUpdate = {};
    if (packed.position !== undefined) {
        update.position = unpack_position(packed.position);
    }
    if (packed.radius !== undefined) {
        update.radius = packed.radius;
    }
    return update;
}

function pack_ship_update(update: ShipUpdate): ShipUpdatePacked {
    const packed: ShipUpdatePacked = {};
    if (update.position !== undefined) {
        packed.position = pack_position(update.position);
    }
    return packed;
}

function unpack_ship_update(packed: ShipUpdatePacked): ShipUpdate {
    const update: ShipUpdate = {};
    if (packed.position !== undefined) {
        update.position = unpack_position(packed.position);
    }
    return update;
}
