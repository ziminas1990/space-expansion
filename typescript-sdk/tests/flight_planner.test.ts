import { expect, test } from "vitest";
import type { Position, Vector } from "../types/index.js";
import { build_plan, type FlightPlan, replay_flight_plan, type Ship,
    verify_flight_plan } from "../utils/flight_planner.js";
import { predict_position } from "../utils/predictor.js";
import { Randomizer } from "./helpers/index.js";

export type FlightPlanner = (ship: Ship, target: Position) => FlightPlan;

type Status = {
    passed: true;
} | {
    passed: false;
    error: string;
}

function run_test(
    ship: Ship,
    target: Position,
    planner: FlightPlanner,
    max_distance: number = 10,
    max_velocity_diff: number = 0.1,
): Status
{
    const plan = planner(ship, target);
    if (!verify_flight_plan(ship, plan)) {
        return { passed: false, error: "invalid flight plan" };
    }
    const final_ship = replay_flight_plan(ship, plan);
    const target_at_arrival = predict_position(target, final_ship.position.timestamp);
    const distance = Math.hypot(
        final_ship.position.point[0] - target_at_arrival.point[0],
        final_ship.position.point[1] - target_at_arrival.point[1]);
    const velocity_diff = Math.hypot(
        final_ship.position.velocity[0] - target.velocity[0],
        final_ship.position.velocity[1] - target.velocity[1]);

    if (!Number.isFinite(distance) || distance > max_distance) {
        return { passed: false, error: `distance is too large: ${distance}` };
    }
    if (!Number.isFinite(velocity_diff) || velocity_diff > max_velocity_diff) {
        return { passed: false, error: `velocity diff is too large: ${velocity_diff}` };
    }
    return { passed: true };
}

function random_kinematic(rng: Randomizer, timestamp: number): Position {
    const position = rng.randomPosition({
        circle: { center: [0, 0], radius: 100_000 },
        maxSpeed: 5_000,
    });
    position.timestamp = timestamp;
    return position;
}

function random_orientation(rng: Randomizer): Vector {
    const angle = rng.randomValue(0, Math.PI * 2);
    return [Math.cos(angle), Math.sin(angle)];
}

function target_has_no_velocity(rng: Randomizer): boolean {
    return rng.randomValue(0, 1) < 0.1;
}

function ship_has_no_velocity(rng: Randomizer): boolean {
    return rng.randomValue(0, 1) < 0.1;
}

export function test_one_case(seed: number, planner: FlightPlanner): Status  {
    const rng = new Randomizer(seed);

    const now = rng.randomInt(0, 10_000_000);
    const target = random_kinematic(rng, now);
    if (target_has_no_velocity(rng)) {
        target.velocity = [0, 0];
    }

    const ship_position = random_kinematic(rng, now);
    if (ship_has_no_velocity(rng)) {
        ship_position.velocity = [0, 0];
    }

    const ship_mass = rng.randomValue(1000, 100000);
    const max_acc = rng.randomValue(0.3, 10);
    const max_thrust = ship_mass * max_acc;

    const ship: Ship = {
        mass: ship_mass,
        position: ship_position,
        orientation: random_orientation(rng),
        engine_max_thrust: max_thrust,
        max_rotate_speed: rng.randomValue(0.1, 6),
    }

    try {
        const status = run_test(ship, target, planner);
        return status.passed ? status : { passed: false, error: `seed ${seed}: ${status.error}` };
    } catch (error) {
        return { passed: false, error: `seed ${seed}: ${String(error)}` };
    }
}

export function test_multiple_cases(total: number = 10000, planner: FlightPlanner, seed = Date.now())
: Status
{
    const seeds_generator = new Randomizer(seed);

    for (let i = 0; i < total; i += 1) {
        const seed = seeds_generator.randomInt(0, 1_000_000_000);
        const status = test_one_case(seed, planner);
        if (!status.passed) {
            return status;
        }
    }
    return { passed: true };
}

test("intercepts 10,000 moving targets", () => {
    const status = test_multiple_cases(10_000, build_plan, 20260924);
    expect(status, JSON.stringify(status)).toEqual({ passed: true });
}, 120_000);

function make_ship(overrides: Partial<Ship> = {}): Ship {
    return {
        mass: 1000,
        position: { timestamp: 3_000_000, point: [0, 0], velocity: [0, 0] },
        orientation: [1, 0],
        engine_max_thrust: 1000,
        max_rotate_speed: Math.PI,
        ...overrides,
    };
}

test("approaches the analytic straight transfer time including the flip and arrival tolerances", () => {
    const ship = make_ship();
    const target: Position = { timestamp: 0, point: [1000, 0], velocity: [0, 0] };
    const plan = build_plan(ship, target);
    const final = replay_flight_plan(ship, plan);
    // With terminal speed u and distance d: T = sqrt(4*d/a + flip^2
    // + 2*(u/a)^2) - u/a. The tolerances permit d=990 and u=0.1.
    const optimal = Math.sqrt(4 * 990 + 1 + 2 * 0.1 ** 2) - 0.1;
    expect((final.position.timestamp - ship.position.timestamp) / 1e6)
        .toBeCloseTo(optimal, 3);
    expect(plan.maneuvers).toHaveLength(3);
    expect(run_test(ship, target, build_plan)).toEqual({ passed: true });
});

test.each([
    { point: [0, 0], velocity: [200, -300], orientation: [1, 0], turn: 0.1 },
    { point: [1000, 0], velocity: [-500, 0], orientation: [-1, 0], turn: 0.1 },
    { point: [0, 1000], velocity: [500, 0], orientation: [1, 0], turn: 6 },
    { point: [0, -1000], velocity: [0, 0], orientation: [0, 2], turn: 0.001 },
    { point: [10.01, 0], velocity: [0, 0], orientation: [-1, 0], turn: 0.1 },
] as { point: Vector; velocity: Vector; orientation: Vector; turn: number }[])
("handles coincident, receding, transverse and slowly turning states: %j", data => {
    const ship = make_ship({
        position: { timestamp: 3_000_000, point: [0, 0], velocity: data.velocity },
        orientation: data.orientation, max_rotate_speed: data.turn,
    });
    const target: Position = { timestamp: 0, point: data.point, velocity: [0, 0] };
    expect(run_test(ship, target, build_plan)).toEqual({ passed: true });
});

test("aligns timestamps and does not mutate its inputs", () => {
    const ship = make_ship({ orientation: [-2, 3], max_rotate_speed: 0.1 });
    const target: Position = { timestamp: -5_000_000, point: [100, -300], velocity: [200, 700] };
    const before = structuredClone({ ship, target });
    expect(run_test(ship, target, build_plan)).toEqual({ passed: true });
    expect({ ship, target }).toEqual(before);
});

test("does nothing when already within both arrival tolerances", () => {
    const ship = make_ship({ engine_max_thrust: 0, max_rotate_speed: 0 });
    const target: Position = { timestamp: ship.position.timestamp, point: [10, 0], velocity: [0.1, 0] };
    expect(build_plan(ship, target)).toEqual({ maneuvers: [] });
});

test("uses a short burn and coast instead of an expensive braking turn", () => {
    const ship = make_ship({ max_rotate_speed: 0.001 });
    const target: Position = { timestamp: 0, point: [11, 0], velocity: [0, 0] };
    const plan = build_plan(ship, target);
    const final = replay_flight_plan(ship, plan);
    expect((final.position.timestamp - ship.position.timestamp) / 1e6).toBeLessThan(10.07);
    expect(plan.maneuvers.every(item => item.maneuver.kind === "thrust")).toBe(true);
    expect(run_test(ship, target, build_plan)).toEqual({ passed: true });
});

test("can arrive ballistically without propulsion", () => {
    const ship = make_ship({ engine_max_thrust: 0, max_rotate_speed: 0,
        position: { timestamp: 0, point: [0, 0], velocity: [0.05, 0] } });
    const target: Position = { timestamp: 0, point: [10.5, 0], velocity: [0, 0] };
    expect(run_test(ship, target, build_plan)).toEqual({ passed: true });
    const final = replay_flight_plan(ship, build_plan(ship, target));
    expect(final.position.timestamp / 1e6).toBeLessThan(10.03);
});

test("can use its current heading when rotation is unavailable", () => {
    const ship = make_ship({ max_rotate_speed: 0 });
    const target: Position = { timestamp: 0, point: [11, 0], velocity: [0, 0] };
    expect(run_test(ship, target, build_plan)).toEqual({ passed: true });
});

test("rejects non-finite inputs and unavailable propulsion for an active transfer", () => {
    const target: Position = { timestamp: 0, point: [1000, 0], velocity: [0, 0] };
    for (const overrides of [{ mass: 0 }, { max_rotate_speed: 0, orientation: [-1, 0] as Vector },
        { engine_max_thrust: 0 }, { mass: NaN }, { orientation: [0, 0] as Vector }])
    {
        expect(() => build_plan(make_ship(overrides), target)).toThrow(RangeError);
    }
});
