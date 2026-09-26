import { expect, test } from "vitest";
import { Asteroid } from "./asteroid.js";
import { PlayerShip } from "./player_ship.js";
import type { Position } from "./position.js";
import { Ship } from "./ship.js";
import { World } from "./world.js";
import { noop_logger } from "../logger.js";

function sample_position(timestamp: number, x = 0, y = 0): Position {
    return {
        timestamp,
        x,
        y,
        velocity: { x: 10, y: -4 },
        acc: { x: 1, y: 2 },
    };
}

test("round-trips a packed asteroid snapshot", () => {
    // 1. pack a stale asteroid
    const asteroid = new Asteroid(
        "rock-1",
        sample_position(1_000_000, 5, 6),
        40,
        { x: 1, y: 0 },
    );
    asteroid.outdated = true;
    const packed = asteroid.pack();

    // 2. unpack through json and read the same fields back
    const restored = Asteroid.unpack(JSON.parse(JSON.stringify(packed)));
    expect(restored).toBeInstanceOf(Asteroid);
    expect(restored.get_id()).toBe("rock-1");
    expect(restored.get_radius()).toBe(40);
    expect(restored.outdated).toBe(true);
    expect(restored.get_position()).toEqual(sample_position(1_000_000, 5, 6));
    expect(restored.get_orientation()).toEqual({ x: 1, y: 0 });
    expect(restored.pack()).toEqual(packed);
});

test("round-trips a packed detected ship snapshot", () => {
    // 1. pack a detected ship
    const ship = new Ship("foreign-1", sample_position(2_000_000, 1, 2));
    const packed = ship.pack();

    // 2. unpack and confirm identity, position, and outdated flag
    const restored = Ship.unpack(JSON.parse(JSON.stringify(packed)));
    expect(restored).toBeInstanceOf(Ship);
    expect(restored).not.toBeInstanceOf(PlayerShip);
    expect(restored.get_id()).toBe("foreign-1");
    expect(restored.outdated).toBe(false);
    expect(restored.get_position()).toEqual(sample_position(2_000_000, 1, 2));
    expect(restored.get_orientation()).toBeUndefined();
    expect(restored.pack()).toEqual(packed);
});

test("round-trips a packed player ship snapshot", () => {
    // 1. pack a standalone player ship
    const ship = new PlayerShip(
        "Scout",
        sample_position(3_000_000, 100, 200),
        25,
        "Tiny-Scout",
        { x: 0, y: 1 },
    );
    ship.outdated = true;
    const packed = ship.pack();

    // 2. unpack as PlayerShip, not as a detected Ship
    const restored = PlayerShip.unpack(JSON.parse(JSON.stringify(packed)));
    expect(restored).toBeInstanceOf(PlayerShip);
    expect(restored).not.toBeInstanceOf(Ship);
    expect(restored.get_id()).toBe("Scout");
    expect(restored.outdated).toBe(true);
    expect(restored.get_position()).toEqual(sample_position(3_000_000, 100, 200));
    expect(restored.get_radius()).toBe(25);
    expect(restored.get_blueprint_name()).toBe("Tiny-Scout");
    expect(restored.get_orientation()).toEqual({
        timestamp: 3_000_000,
        x: 0,
        y: 1,
        turn: 0,
    });
    expect(restored.pack()).toEqual(packed);
});

test("round-trips a packed world snapshot", () => {
    // 1. build a world with one of each entity
    const world = new World(noop_logger, 12_000_000);
    const asteroid = new Asteroid("rock-1", sample_position(1_000_000, 5, 6), 40);
    asteroid.outdated = true;
    world.update({ type: "add_asteroid", asteroid });
    world.update({
        type: "add_ship",
        ship: new Ship("foreign-1", sample_position(2_000_000, 1, 2)),
    });
    world.update({
        type: "add_player_ship",
        ship: new PlayerShip("Scout", sample_position(3_000_000, 100, 200), 25, "Tiny-Scout"),
    });

    // 2. unpack a json-encoded snapshot into a new world
    const packed = world.pack();
    const restored = World.unpack(JSON.parse(JSON.stringify(packed)), noop_logger);

    // 3. look up each entity through the read apis
    expect(restored.get_asteroid("rock-1")?.get_radius()).toBe(40);
    expect(restored.get_asteroid("rock-1")?.outdated).toBe(true);
    expect(restored.get_detected_ship("foreign-1")?.get_position().x).toBe(1);
    const player = restored.get_player_ship("Scout");
    expect(player).toBeInstanceOf(PlayerShip);
    expect(player).not.toBeInstanceOf(Ship);
    expect(player?.get_position().y).toBe(200);
    expect(player?.get_radius()).toBe(25);
    expect(player?.get_blueprint_name()).toBe("Tiny-Scout");
    expect(restored.get_asteroids()).toHaveLength(1);
    expect(restored.get_detected_ships()).toHaveLength(1);
    expect(restored.get_player_ships()).toHaveLength(1);
    expect(restored.pack()).toEqual(packed);
});

test("keeps a known orientation and leaves an unknown one unset", () => {
    // 1. add an asteroid with no facing and a ship facing +X
    const world = new World(noop_logger);
    world.update({
        type: "add_asteroid",
        asteroid: new Asteroid("rock-1", sample_position(1_000_000, 5, 6), 40),
    });
    world.update({
        type: "add_player_ship",
        ship: new PlayerShip("Scout", sample_position(2_000_000), 25, "Tiny-Scout", { x: 1, y: 0 }),
    });
    expect(world.get_asteroid("rock-1")?.get_orientation()).toBeUndefined();
    expect(world.get_player_ship("Scout")?.get_orientation()).toEqual({
        timestamp: 2_000_000,
        x: 1,
        y: 0,
        turn: 0,
    });

    // 2. give the asteroid a facing and turn the ship to +Y
    world.update({
        type: "asteroid_update",
        asteroid_id: "rock-1",
        update: { orientation: { x: 1, y: 0 } },
    });
    world.update({
        type: "player_ship_update",
        ship_id: "Scout",
        update: { orientation: { x: 0, y: 1 } },
    });

    // 3. read both directions back
    expect(world.get_asteroid("rock-1")?.get_orientation()).toEqual({ x: 1, y: 0 });
    expect(world.get_player_ship("Scout")?.get_orientation()).toEqual({
        timestamp: 2_000_000,
        x: 0,
        y: 1,
        turn: 0,
    });
    expect(world.get_player_ship("Scout")?.get_radius()).toBe(25);
});
