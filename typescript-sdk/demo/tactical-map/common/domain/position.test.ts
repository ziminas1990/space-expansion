import { expect, test } from "vitest";
import {
    predict_position,
    update_position,
    type Position,
} from "./position.js";
import { Ship } from "./ship.js";

function sample_position(overrides: Partial<Position> = {}): Position {
    return {
        timestamp: 0,
        x: 0,
        y: 0,
        velocity: { x: 10, y: -4 },
        acc: { x: 2, y: 6 },
        ...overrides,
    };
}

test("predicts position and velocity with constant acceleration", () => {
    // 1. start from a position with a known acceleration
    const position = sample_position();

    // 2. predict half a second ahead
    const predicted = predict_position(position, 500_000);

    // 3. position uses 0.5*a*dt^2, velocity integrates a*dt
    expect(predicted.timestamp).toBe(500_000);
    expect(predicted.x).toBeCloseTo(5.25);
    expect(predicted.y).toBeCloseTo(-1.25);
    expect(predicted.velocity.x).toBeCloseTo(11);
    expect(predicted.velocity.y).toBeCloseTo(-1);
    expect(predicted.acc).toEqual(position.acc);
});

test("estimates acceleration from a velocity change", () => {
    // 1. take two samples one second apart
    const previous = sample_position({
        timestamp: 0,
        velocity: { x: 10, y: 0 },
        acc: { x: 0, y: 0 },
    });
    const next = sample_position({
        timestamp: 1_000_000,
        x: 11,
        y: 3,
        velocity: { x: 12, y: 4 },
        acc: { x: 0, y: 0 },
    });

    // 2. derive acc as dv/dt
    const estimated = update_position(previous, next);
    expect(estimated.timestamp).toBe(1_000_000);
    expect(estimated.x).toBe(11);
    expect(estimated.y).toBe(3);
    expect(estimated.velocity).toEqual({ x: 12, y: 4 });
    expect(estimated.acc.x).toBeCloseTo(2);
    expect(estimated.acc.y).toBeCloseTo(4);

    // 3. the inputs stay unchanged
    expect(previous.acc).toEqual({ x: 0, y: 0 });
    expect(next.acc).toEqual({ x: 0, y: 0 });
});

test("estimates acceleration when a ship receives a later position", () => {
    // 1. spawn a ship with a known velocity
    const ship = new Ship("foreign-1", sample_position({
        timestamp: 0,
        velocity: { x: 10, y: -4 },
        acc: { x: 0, y: 0 },
    }));

    // 2. apply a later sample whose velocity has changed
    ship.update({
        position: sample_position({
            timestamp: 500_000,
            x: 6,
            y: -1,
            velocity: { x: 12, y: -3 },
            acc: { x: 0, y: 0 },
        }),
    });

    // 3. acc is taken from the velocity difference
    const position = ship.get_position();
    expect(position.x).toBe(6);
    expect(position.acc.x).toBeCloseTo(4);
    expect(position.acc.y).toBeCloseTo(2);
});
