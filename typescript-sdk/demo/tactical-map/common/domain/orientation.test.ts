import { expect, test } from "vitest";
import { PlayerShip } from "./player_ship.js";
import {
    predict_orientation,
    update_orientation,
    type Orientation,
} from "./orientation.js";
import type { Position } from "./position.js";
import { Ship } from "./ship.js";

function sample_position(overrides: Partial<Position> = {}): Position {
    return {
        timestamp: 0,
        x: 0,
        y: 0,
        velocity: { x: 10, y: -4 },
        acc: { x: 0, y: 0 },
        ...overrides,
    };
}

function facing(
    timestamp: number,
    x: number,
    y: number,
    turn = 0,
): Orientation {
    return { timestamp, x, y, turn };
}

test("predicts facing with a constant turn rate", () => {
    // 1. face +X while turning toward +Y at a quarter turn per second
    const orientation = facing(0, 1, 0, Math.PI / 2);

    // 2. predict half a second ahead
    const predicted = predict_orientation(orientation, 500_000);

    // 3. the nose is halfway to +Y, and the rate is unchanged
    expect(predicted.timestamp).toBe(500_000);
    expect(predicted.x).toBeCloseTo(Math.SQRT1_2);
    expect(predicted.y).toBeCloseTo(Math.SQRT1_2);
    expect(predicted.turn).toBeCloseTo(Math.PI / 2);
    expect(orientation).toEqual(facing(0, 1, 0, Math.PI / 2));
});

test("estimates turn rate from a facing change", () => {
    // 1. take two samples one second apart, from +X to +Y
    const previous = facing(0, 1, 0);
    const next = facing(1_000_000, 0, 1);

    // 2. derive turn as the shortest angle over ingame time
    const estimated = update_orientation(previous, next);
    expect(estimated.timestamp).toBe(1_000_000);
    expect(estimated.x).toBe(0);
    expect(estimated.y).toBe(1);
    expect(estimated.turn).toBeCloseTo(Math.PI / 2);

    // 3. the opposite direction is a negative rate
    const reversed = update_orientation(previous, facing(1_000_000, 0, -1));
    expect(reversed.turn).toBeCloseTo(-Math.PI / 2);

    // 4. the inputs stay unchanged
    expect(previous).toEqual(facing(0, 1, 0));
    expect(next).toEqual(facing(1_000_000, 0, 1));
});

test("takes the short way across the branch cut", () => {
    // 1. step from just above -X on the +Y side to just below it
    const from_angle = Math.PI - 0.1;
    const to_angle = -Math.PI + 0.1;
    const previous = facing(0, Math.cos(from_angle), Math.sin(from_angle));
    const next = facing(
        1_000_000,
        Math.cos(to_angle),
        Math.sin(to_angle),
    );

    // 2. the turn is the short positive step, not almost a full circle
    const estimated = update_orientation(previous, next);
    expect(estimated.turn).toBeCloseTo(0.2);
});

test("keeps an older facing and stops when the nose no longer moves", () => {
    // 1. a ship that was turning receives an older sample
    const current = facing(1_000_000, 0, 1, Math.PI / 2);
    const kept = update_orientation(current, facing(0, 1, 0));
    expect(kept).toEqual(current);

    // 2. the same nose a second later means the ship stopped turning
    const stopped = update_orientation(current, facing(2_000_000, 0, 1));
    expect(stopped.turn).toBe(0);
    expect(stopped.x).toBe(0);
    expect(stopped.y).toBe(1);
});

test("estimates turn rate when a ship receives a later facing", () => {
    // 1. spawn one ship of each kind, both facing +X
    const detected = new Ship(
        "foreign-1",
        sample_position(),
        { x: 1, y: 0 },
    );
    const player = new PlayerShip(
        "Scout",
        sample_position(),
        25,
        "Tiny-Scout",
        { x: 1, y: 0 },
    );

    // 2. a second later both face +Y
    for (const ship of [detected, player]) {
        ship.update({
            position: sample_position({ timestamp: 1_000_000, x: 6, y: -1 }),
            orientation: { x: 0, y: 1 },
        });
    }

    // 3. both store a quarter turn per second, and a snapshot keeps it
    expect(detected.get_orientation()?.turn).toBeCloseTo(Math.PI / 2);
    expect(player.get_orientation()?.turn).toBeCloseTo(Math.PI / 2);
    const restored = Ship.unpack(JSON.parse(JSON.stringify(detected.pack())));
    expect(restored.get_orientation()).toEqual(detected.get_orientation());
    const restored_player = PlayerShip.unpack(
        JSON.parse(JSON.stringify(player.pack())),
    );
    expect(restored_player.get_orientation()).toEqual(player.get_orientation());
});
