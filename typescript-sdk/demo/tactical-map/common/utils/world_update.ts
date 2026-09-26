import { Asteroid, AsteroidPacked, AsteroidUpdate } from "../domain/asteroid.js";
import { PlayerShip, PlayerShipPacked } from "../domain/player_ship.js";
import type { ModuleInfoPacked } from "../domain/module.js";
import type { ContainerContent } from "../domain/resource_container.js";
import type { RCSThrust } from "../domain/rcs.js";
import {
    pack_position,
    pack_vector,
    unpack_position,
    unpack_vector,
    VectorPacked,
} from "../domain/position.js";
import { Ship, ShipPacked, ShipUpdate } from "../domain/ship.js";
import { EntityRef, WorldUpdate } from "../domain/world.js";

export type PositionPacked = ReturnType<typeof pack_position>;

const packed_update_type = {
    add_asteroid: 0,
    add_ship: 1,
    add_player_ship: 2,
    asteroid_update: 3,
    ship_update: 4,
    player_ship_update: 5,
    remove_entity: 6,
    player_ship_modules_update: 7,
    resource_container_update: 8,
    hover_engine_update: 9,
    rcs_update: 10,
} as const;

const packed_entity_kind = {
    asteroid: 0,
    ship: 1,
    player_ship: 2,
} as const;

type PackedEntityKind =
    typeof packed_entity_kind[keyof typeof packed_entity_kind];

export type AsteroidUpdatePacked = [
    PositionPacked | null,
    number | null,
    VectorPacked | null,
];

export type ShipUpdatePacked = [
    PositionPacked | null,
    VectorPacked | null,
];

export type WorldUpdatePacked =
    | [typeof packed_update_type.add_asteroid, AsteroidPacked]
    | [typeof packed_update_type.add_ship, ShipPacked]
    | [typeof packed_update_type.add_player_ship, PlayerShipPacked]
    | [typeof packed_update_type.asteroid_update, string, ...AsteroidUpdatePacked]
    | [typeof packed_update_type.ship_update, string, ...ShipUpdatePacked]
    | [typeof packed_update_type.player_ship_update, string, ...ShipUpdatePacked]
    | [typeof packed_update_type.remove_entity, PackedEntityKind, string]
    | [typeof packed_update_type.player_ship_modules_update, string, ModuleInfoPacked[]]
    | [typeof packed_update_type.resource_container_update, string, number, ContainerContent]
    | [typeof packed_update_type.hover_engine_update, string, number, number]
    | [typeof packed_update_type.rcs_update, string, number, RCSThrust];

export function pack_world_update(update: WorldUpdate): WorldUpdatePacked {
    switch (update.type) {
        case "add_asteroid":
            return [packed_update_type.add_asteroid, update.asteroid.pack()];
        case "add_ship":
            return [packed_update_type.add_ship, update.ship.pack()];
        case "add_player_ship":
            return [packed_update_type.add_player_ship, update.ship.pack()];
        case "asteroid_update": {
            const packed_update = pack_asteroid_update(update.update);
            return [
                packed_update_type.asteroid_update,
                update.asteroid_id,
                packed_update[0],
                packed_update[1],
                packed_update[2],
            ];
        }
        case "ship_update": {
            const packed_update = pack_ship_update(update.update);
            return [
                packed_update_type.ship_update,
                update.ship_id,
                packed_update[0],
                packed_update[1],
            ];
        }
        case "player_ship_update": {
            const packed_update = pack_ship_update(update.update);
            return [
                packed_update_type.player_ship_update,
                update.ship_id,
                packed_update[0],
                packed_update[1],
            ];
        }
        case "player_ship_modules_update":
            return [
                packed_update_type.player_ship_modules_update,
                update.ship_id,
                update.modules.map((module): ModuleInfoPacked => [
                    module.slot_id, module.type, module.name,
                ]),
            ];
        case "resource_container_update":
            return [
                packed_update_type.resource_container_update,
                update.ship_id,
                update.slot_id,
                update.content,
            ];
        case "hover_engine_update":
            return [
                packed_update_type.hover_engine_update,
                update.ship_id,
                update.slot_id,
                update.thrust,
            ];
        case "rcs_update":
            return [
                packed_update_type.rcs_update,
                update.ship_id,
                update.slot_id,
                update.thrust,
            ];
        case "remove_entity":
            return [
                packed_update_type.remove_entity,
                pack_entity_kind(update.entity.kind),
                update.entity.id,
            ];
        default: {
            const unexpected: never = update;
            throw new Error(`Unknown update type: ${(unexpected as WorldUpdate).type}`);
        }
    }
}

export function unpack_world_update(packed: WorldUpdatePacked): WorldUpdate {
    switch (packed[0]) {
        case packed_update_type.add_asteroid:
            return {
                type: "add_asteroid",
                asteroid: Asteroid.unpack(packed[1]),
            };
        case packed_update_type.add_ship:
            return {
                type: "add_ship",
                ship: Ship.unpack(packed[1]),
            };
        case packed_update_type.add_player_ship:
            return {
                type: "add_player_ship",
                ship: PlayerShip.unpack(packed[1]),
            };
        case packed_update_type.asteroid_update:
            return {
                type: "asteroid_update",
                asteroid_id: packed[1],
                update: unpack_asteroid_update(packed[2], packed[3], packed[4]),
            };
        case packed_update_type.ship_update:
            return {
                type: "ship_update",
                ship_id: packed[1],
                update: unpack_ship_update(packed[2], packed[3]),
            };
        case packed_update_type.player_ship_update:
            return {
                type: "player_ship_update",
                ship_id: packed[1],
                update: unpack_ship_update(packed[2], packed[3]),
            };
        case packed_update_type.player_ship_modules_update:
            return {
                type: "player_ship_modules_update",
                ship_id: packed[1],
                modules: packed[2].map(([slot_id, type, name]) => ({
                    slot_id,
                    type,
                    name,
                })),
            };
        case packed_update_type.resource_container_update:
            return {
                type: "resource_container_update",
                ship_id: packed[1],
                slot_id: packed[2],
                content: packed[3],
            };
        case packed_update_type.hover_engine_update:
            return {
                type: "hover_engine_update",
                ship_id: packed[1],
                slot_id: packed[2],
                thrust: packed[3],
            };
        case packed_update_type.rcs_update:
            return {
                type: "rcs_update",
                ship_id: packed[1],
                slot_id: packed[2],
                thrust: packed[3],
            };
        case packed_update_type.remove_entity:
            return {
                type: "remove_entity",
                entity: {
                    kind: unpack_entity_kind(packed[1]),
                    id: packed[2],
                },
            };
        default: {
            const unexpected: never = packed;
            throw new Error(`Unknown update type: ${(unexpected as WorldUpdatePacked)[0]}`);
        }
    }
}

function pack_asteroid_update(update: AsteroidUpdate): AsteroidUpdatePacked {
    return [
        update.position !== undefined ? pack_position(update.position) : null,
        update.radius !== undefined ? update.radius : null,
        pack_vector(update.orientation),
    ];
}

function unpack_asteroid_update(
    position: PositionPacked | null,
    radius: number | null,
    orientation: VectorPacked | null,
): AsteroidUpdate {
    const update: AsteroidUpdate = {};
    if (position !== null && position !== undefined) {
        update.position = unpack_position(position);
    }
    if (radius !== null && radius !== undefined) {
        update.radius = radius;
    }
    const facing = unpack_vector(orientation);
    if (facing !== undefined) {
        update.orientation = facing;
    }
    return update;
}

function pack_ship_update(update: ShipUpdate): ShipUpdatePacked {
    return [
        update.position !== undefined ? pack_position(update.position) : null,
        pack_vector(update.orientation),
    ];
}

function unpack_ship_update(
    position: PositionPacked | null,
    orientation: VectorPacked | null,
): ShipUpdate {
    const update: ShipUpdate = {};
    if (position !== null && position !== undefined) {
        update.position = unpack_position(position);
    }
    const facing = unpack_vector(orientation);
    if (facing !== undefined) {
        update.orientation = facing;
    }
    return update;
}

function pack_entity_kind(kind: EntityRef["kind"]): PackedEntityKind {
    switch (kind) {
        case "asteroid":
            return packed_entity_kind.asteroid;
        case "ship":
            return packed_entity_kind.ship;
        case "player_ship":
            return packed_entity_kind.player_ship;
        default: {
            const unexpected: never = kind;
            throw new Error(`Unknown entity kind: ${unexpected as string}`);
        }
    }
}

function unpack_entity_kind(packed: PackedEntityKind): EntityRef["kind"] {
    switch (packed) {
        case packed_entity_kind.asteroid:
            return "asteroid";
        case packed_entity_kind.ship:
            return "ship";
        case packed_entity_kind.player_ship:
            return "player_ship";
        default: {
            const unexpected: never = packed;
            throw new Error(`Unknown entity kind: ${unexpected as number}`);
        }
    }
}
