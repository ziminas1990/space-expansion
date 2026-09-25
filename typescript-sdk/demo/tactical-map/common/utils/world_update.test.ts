import { expect, test } from "vitest";
import { Asteroid } from "../domain/asteroid.js";
import { PlayerShip } from "../domain/player_ship.js";
import { pack_position, type Position } from "../domain/position.js";
import { Ship } from "../domain/ship.js";
import type { WorldUpdate } from "../domain/world.js";
import { pack_world_update, unpack_world_update } from "./world_update.js";

function sample_position(timestamp: number, x = 0, y = 0): Position {
    return {
        timestamp,
        x,
        y,
        velocity: { x: 3, y: 4 },
        acc: { x: 0, y: 1 },
    };
}

function round_trip(update: WorldUpdate): WorldUpdate {
    const packed = pack_world_update(update);
    const via_json = JSON.parse(JSON.stringify(packed));
    const unpacked = unpack_world_update(via_json);
    expect(pack_world_update(unpacked)).toEqual(packed);
    return unpacked;
}

test("packs world updates as numeric tuples", () => {
    // 1. pack add, partial-update, and remove variants
    const asteroid = new Asteroid("rock-1", sample_position(1_000_000, 5, 6), 40);
    const add_asteroid = pack_world_update({ type: "add_asteroid", asteroid });
    const radius_only = pack_world_update({
        type: "asteroid_update",
        asteroid_id: "rock-1",
        update: { radius: 12 },
    });
    const ship_update = pack_world_update({
        type: "ship_update",
        ship_id: "foreign-1",
        update: { position: sample_position(5_000_000, 11, 12) },
    });
    const remove_asteroid = pack_world_update({
        type: "remove_entity",
        entity: { kind: "asteroid", id: "rock-1" },
    });
    const remove_ship = pack_world_update({
        type: "remove_entity",
        entity: { kind: "ship", id: "foreign-1" },
    });
    const remove_player = pack_world_update({
        type: "remove_entity",
        entity: { kind: "player_ship", id: "Scout" },
    });

    // 2. check the wire tuples
    expect(add_asteroid).toEqual([0, asteroid.pack()]);
    expect(radius_only).toEqual([3, "rock-1", null, 12, null]);
    expect(ship_update).toEqual([
        4,
        "foreign-1",
        pack_position(sample_position(5_000_000, 11, 12)),
        null,
    ]);
    expect(remove_asteroid).toEqual([6, 0, "rock-1"]);
    expect(remove_ship).toEqual([6, 1, "foreign-1"]);
    expect(remove_player).toEqual([6, 2, "Scout"]);
});

test("round-trips every world update variant", () => {
    const asteroid = new Asteroid("rock-1", sample_position(1_000_000, 5, 6), 40);
    asteroid.outdated = true;
    const ship = new Ship("foreign-1", sample_position(2_000_000, 8, 9));
    const player = new PlayerShip("Scout", sample_position(3_000_000, 100, 200), 25);
    const variants: WorldUpdate[] = [
        { type: "add_asteroid", asteroid },
        { type: "add_ship", ship },
        { type: "add_player_ship", ship: player },
        {
            type: "asteroid_update",
            asteroid_id: "rock-1",
            update: {
                position: sample_position(4_000_000, 7, 8),
                radius: 50,
                orientation: { x: 1, y: 0 },
            },
        },
        {
            type: "asteroid_update",
            asteroid_id: "rock-1",
            update: { radius: 12 },
        },
        {
            type: "ship_update",
            ship_id: "foreign-1",
            update: { position: sample_position(5_000_000, 11, 12) },
        },
        {
            type: "player_ship_update",
            ship_id: "Scout",
            update: {
                position: sample_position(6_000_000, 13, 14),
                orientation: { x: 0, y: 1 },
            },
        },
        { type: "remove_entity", entity: { kind: "asteroid", id: "rock-1" } },
        { type: "remove_entity", entity: { kind: "ship", id: "foreign-1" } },
        { type: "remove_entity", entity: { kind: "player_ship", id: "Scout" } },
    ];

    // 1. pack and unpack each update through json
    const unpacked: WorldUpdate[] = [];
    for (const update of variants) {
        unpacked.push(round_trip(update));
    }

    // 2. read nested fields back from the restored updates

    const added_asteroid = unpacked[0];
    expect(added_asteroid?.type).toBe("add_asteroid");
    if (added_asteroid?.type === "add_asteroid") {
        expect(added_asteroid.asteroid).toBeInstanceOf(Asteroid);
        expect(added_asteroid.asteroid.get_id()).toBe("rock-1");
        expect(added_asteroid.asteroid.get_radius()).toBe(40);
        expect(added_asteroid.asteroid.outdated).toBe(true);
    }

    const added_ship = unpacked[1];
    expect(added_ship?.type).toBe("add_ship");
    if (added_ship?.type === "add_ship") {
        expect(added_ship.ship).toBeInstanceOf(Ship);
        expect(added_ship.ship.get_id()).toBe("foreign-1");
    }

    const added_player = unpacked[2];
    expect(added_player?.type).toBe("add_player_ship");
    if (added_player?.type === "add_player_ship") {
        expect(added_player.ship).toBeInstanceOf(PlayerShip);
        expect(added_player.ship).not.toBeInstanceOf(Ship);
        expect(added_player.ship.get_id()).toBe("Scout");
        expect(added_player.ship.get_radius()).toBe(25);
    }

    const asteroid_update = unpacked[3];
    expect(asteroid_update?.type).toBe("asteroid_update");
    if (asteroid_update?.type === "asteroid_update") {
        expect(asteroid_update.asteroid_id).toBe("rock-1");
        expect(asteroid_update.update.radius).toBe(50);
        expect(asteroid_update.update.position).toEqual(sample_position(4_000_000, 7, 8));
        expect(asteroid_update.update.orientation).toEqual({ x: 1, y: 0 });
    }

    const radius_only = unpacked[4];
    expect(radius_only?.type).toBe("asteroid_update");
    if (radius_only?.type === "asteroid_update") {
        expect(radius_only.update.position).toBeUndefined();
        expect(radius_only.update.radius).toBe(12);
        expect(radius_only.update.orientation).toBeUndefined();
    }

    const ship_update = unpacked[5];
    expect(ship_update?.type).toBe("ship_update");
    if (ship_update?.type === "ship_update") {
        expect(ship_update.update.position).toEqual(sample_position(5_000_000, 11, 12));
    }

    const player_update = unpacked[6];
    expect(player_update?.type).toBe("player_ship_update");
    if (player_update?.type === "player_ship_update") {
        expect(player_update.update.position).toEqual(sample_position(6_000_000, 13, 14));
        expect(player_update.update.orientation).toEqual({ x: 0, y: 1 });
    }

    expect(unpacked[7]).toEqual({
        type: "remove_entity",
        entity: { kind: "asteroid", id: "rock-1" },
    });
    expect(unpacked[8]).toEqual({
        type: "remove_entity",
        entity: { kind: "ship", id: "foreign-1" },
    });
    expect(unpacked[9]).toEqual({
        type: "remove_entity",
        entity: { kind: "player_ship", id: "Scout" },
    });
});
