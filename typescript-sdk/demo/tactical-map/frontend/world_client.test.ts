import { expect, test } from "vitest";
import { Asteroid } from "../common/domain/asteroid.js";
import { PlayerShip } from "../common/domain/player_ship.js";
import type { Position } from "../common/domain/position.js";
import { World } from "../common/domain/world.js";
import { noop_logger } from "../common/logger.js";
import { encode_server_message } from "../common/utils/protocol.js";
import { pack_world_update } from "../common/utils/world_update.js";
import { WorldClient } from "./world_client.js";

function sample_position(timestamp: number, x = 0, y = 0): Position {
    return {
        timestamp,
        x,
        y,
        velocity: { x: 10, y: 0 },
        acc: { x: 0, y: 0 },
    };
}

function packed_world_with_player_ship(): ReturnType<World["pack"]> {
    const world = new World(noop_logger);
    world.update({
        type: "add_player_ship",
        ship: new PlayerShip("Scout", sample_position(1_000_000, 100, 200), 25, "Tiny-Scout"),
    });
    return world.pack();
}

test("applies a packed world snapshot", () => {
    // 1. pack a world that contains a player ship
    const packed = packed_world_with_player_ship();
    const client = new WorldClient({ logger: noop_logger });

    // 2. send the snapshot to the client
    client.handle_message(encode_server_message({
        type: "snapshot",
        world: packed,
    }));

    // 3. read the ship back from the unpacked world
    const ship = client.get_world()?.get_player_ship("Scout");
    expect(ship).toBeDefined();
    expect(ship?.get_position().x).toBe(100);
    expect(ship?.get_position().y).toBe(200);
    expect(ship?.get_radius()).toBe(25);
    expect(ship?.get_blueprint_name()).toBe("Tiny-Scout");
});

test("stops following before clearing the selected ship", () => {
    const client = new WorldClient({ logger: noop_logger });
    client.handle_message(encode_server_message({
        type: "snapshot",
        world: packed_world_with_player_ship(),
    }));

    client.select_ship("Scout");
    expect(client.get_selected_ship_id()).toBe("Scout");
    expect(client.get_followed_ship_id()).toBe("Scout");

    client.stop_following();
    expect(client.get_selected_ship_id()).toBe("Scout");
    expect(client.get_followed_ship_id()).toBeUndefined();

    client.clear_selection();
    expect(client.get_selected_ship_id()).toBeUndefined();
    expect(client.get_followed_ship_id()).toBeUndefined();
});

test("clears a selected ship removed from the world", () => {
    const client = new WorldClient({ logger: noop_logger });
    client.handle_message(encode_server_message({
        type: "snapshot",
        world: packed_world_with_player_ship(),
    }));
    client.select_ship("Scout");
    client.stop_following();

    client.handle_message(encode_server_message({
        type: "world_update",
        update: pack_world_update({
            type: "remove_entity",
            entity: { kind: "player_ship", id: "Scout" },
        }),
    }));
    expect(client.get_selected_ship_id()).toBeUndefined();
});

test("applies a packed world update after the snapshot", () => {
    // 1. load an empty snapshot
    const client = new WorldClient({ logger: noop_logger });
    client.handle_message(encode_server_message({
        type: "snapshot",
        world: new World(noop_logger).pack(),
    }));

    // 2. send an add_asteroid update
    const asteroid = new Asteroid("rock-1", sample_position(2_000_000, 5, 6), 40);
    client.handle_message(encode_server_message({
        type: "world_update",
        update: pack_world_update({
            type: "add_asteroid",
            asteroid,
        }),
    }));

    // 3. look up the asteroid
    const found = client.get_world()?.get_asteroid("rock-1");
    expect(found).toBeDefined();
    expect(found?.get_radius()).toBe(40);
    expect(found?.get_position().x).toBe(5);
});

test("keeps installed modules current across snapshots and updates", () => {
    // 1. load a ship with two installed modules from a snapshot
    const world = new World(noop_logger);
    const ship = new PlayerShip("Scout", sample_position(1_000_000), 25, "Tiny-Scout");
    ship.set_modules([
        { slot_id: 1, type: "RCS", name: "Left RCS" },
        { slot_id: 2, type: "RCS", name: "Right RCS" },
    ]);
    world.update({ type: "add_player_ship", ship });
    const client = new WorldClient({ logger: noop_logger });
    client.handle_message(encode_server_message({
        type: "snapshot",
        world: world.pack(),
    }));
    expect(client.get_world()?.get_player_ship("Scout")?.get_modules()).toEqual([
        { slot_id: 1, type: "RCS", name: "Left RCS" },
        { slot_id: 2, type: "RCS", name: "Right RCS" },
    ]);

    // 2. replace the module list after one RCS is removed and an engine is attached
    const modules = [
        { slot_id: 2, type: "RCS", name: "Right RCS" },
        { slot_id: 3, type: "HoverEngine", name: "Main engine" },
    ];
    client.handle_message(encode_server_message({
        type: "world_update",
        update: pack_world_update({
            type: "player_ship_modules_update",
            ship_id: "Scout",
            modules,
        }),
    }));
    expect(client.get_world()?.get_player_ship("Scout")?.get_modules()).toEqual(modules);
});

test("applies a clock observation using local time", () => {
    // 1. load a snapshot
    const client = new WorldClient({ logger: noop_logger });
    client.handle_message(encode_server_message({
        type: "snapshot",
        world: packed_world_with_player_ship(),
    }));
    expect(client.get_world()?.now()).toBeUndefined();

    // 2. send a clock message
    client.handle_message(encode_server_message({
        type: "clock",
        server_ts: 5_000_000,
        ingame_ts: 9_000_000,
    }));

    // 3. read the world's predicted ingame time
    const now = client.get_world()?.now();
    expect(now).toBeDefined();
    expect(Math.abs((now ?? 0) - 9_000_000)).toBeLessThan(100_000);
});

test("ignores world updates and clock messages before a snapshot", () => {
    // 1. send incremental traffic to an empty client
    const client = new WorldClient({ logger: noop_logger });
    const version_before = client.get_version();
    client.handle_message(encode_server_message({
        type: "world_update",
        update: pack_world_update({
            type: "add_asteroid",
            asteroid: new Asteroid("early", sample_position(1), 1),
        }),
    }));
    client.handle_message(encode_server_message({
        type: "clock",
        server_ts: 1,
        ingame_ts: 1,
    }));

    // 2. confirm the store did not create a world or bump the version
    expect(client.get_world()).toBeUndefined();
    expect(client.get_version()).toBe(version_before);
});

test("notifies subscribers with a monotonically increasing version", () => {
    // 1. subscribe to store changes
    const client = new WorldClient({ logger: noop_logger });
    const versions: number[] = [];
    const unsubscribe = client.subscribe(() => {
        versions.push(client.get_version());
    });

    // 2. apply a snapshot and a later update
    client.handle_message(encode_server_message({
        type: "snapshot",
        world: packed_world_with_player_ship(),
    }));
    client.handle_message(encode_server_message({
        type: "world_update",
        update: pack_world_update({
            type: "add_asteroid",
            asteroid: new Asteroid("rock-2", sample_position(3_000_000), 8),
        }),
    }));

    // 3. unsubscribe and confirm later messages are ignored
    unsubscribe();
    client.handle_message(encode_server_message({
        type: "clock",
        server_ts: 4_000_000,
        ingame_ts: 4_000_000,
    }));

    expect(versions).toEqual([1, 2]);
    expect(client.get_version()).toBe(3);
});
