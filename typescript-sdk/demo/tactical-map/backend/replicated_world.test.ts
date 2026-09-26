import { expect, test } from "vitest";
import { Asteroid } from "../common/domain/asteroid.js";
import { PlayerShip } from "../common/domain/player_ship.js";
import type { Position } from "../common/domain/position.js";
import { Ship } from "../common/domain/ship.js";
import { World } from "../common/domain/world.js";
import { noop_logger } from "../common/logger.js";
import type { ServerMessage } from "../common/utils/protocol.js";
import { unpack_world_update } from "../common/utils/world_update.js";
import { ReplicatedWorld } from "./replicated_world.js";

function sample_position(timestamp: number, x = 0, y = 0): Position {
    return {
        timestamp,
        x,
        y,
        velocity: { x: 0, y: 0 },
        acc: { x: 0, y: 0 },
    };
}

test("keeps updates dormant until snapshot, then sends live updates after it", () => {
    // 1. apply world and clock updates before any remote is attached
    const replica = new ReplicatedWorld(new World(noop_logger));
    const messages: ServerMessage[] = [];
    replica.update({
        type: "add_asteroid",
        asteroid: new Asteroid("rock-1", sample_position(1_000_000, 5, 6), 40),
    });
    replica.update({
        type: "add_player_ship",
        ship: new PlayerShip("Scout", sample_position(2_000_000, 100, 200), 25, "Tiny-Scout"),
    });
    replica.observe(10, 20, 30);
    expect(messages).toEqual([]);

    // 2. attach a remote and record the snapshot-before-live handshake
    const status = replica.add_remote("browser", (message) => {
        messages.push(message);
    });
    expect(status.is_ok()).toBe(true);
    expect(messages.map((message) => message.type)).toEqual(["snapshot", "clock"]);

    const snapshot = messages[0];
    expect(snapshot?.type).toBe("snapshot");
    if (snapshot?.type === "snapshot") {
        const world = World.unpack(snapshot.world, noop_logger);
        expect(world.get_asteroid("rock-1")?.get_radius()).toBe(40);
        expect(world.get_player_ship("Scout")?.get_position().x).toBe(100);
        expect(world.get_player_ship("Scout")?.get_radius()).toBe(25);
        expect(world.get_player_ship("Scout")?.get_blueprint_name()).toBe("Tiny-Scout");
        expect(world.get_detected_ships()).toHaveLength(0);
    }
    expect(messages[1]).toEqual({
        type: "clock",
        server_ts: 20,
        ingame_ts: 30,
    });

    // 3. a later update is sent live after the snapshot, not instead of it
    replica.update({
        type: "add_ship",
        ship: new Ship("foreign-1", sample_position(3_000_000, 8, 9)),
    });
    expect(messages.map((message) => message.type)).toEqual([
        "snapshot",
        "clock",
        "world_update",
    ]);
    const live = messages[2];
    expect(live?.type).toBe("world_update");
    if (live?.type === "world_update") {
        const update = unpack_world_update(live.update);
        expect(update.type).toBe("add_ship");
        if (update.type === "add_ship") {
            expect(update.ship.get_id()).toBe("foreign-1");
        }
    }
});
