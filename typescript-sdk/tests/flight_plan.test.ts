import { expect, test } from "vitest";
import {
    approach_to_plan,
    prepare_flight_plan_in_delta_v,
    prepare_flight_plan_in_time,
    type FlightPlan,
} from "../highlevel/flight_plan.js";
import type { Position } from "../types/index.js";
import { predict_position } from "../utils/predictor.js";
import { vecAbs, vecSub } from "../utils/vector.js";
import { distance, Randomizer } from "./helpers/index.js";

const CASES = 1_000;
const POSITION_DELTA = 5;
const VELOCITY_DELTA = 1;
const INNER_WINDOWS = 3;

function randomKinematic(rng: Randomizer, timestamp: bigint): Position {
    const position = rng.randomPosition({
        center: { timestamp: 0n, point: [0, 0], velocity: [0, 0] },
        radius: 100_000,
        maxSpeed: 5_000,
    });
    if (rng.randomValue(0, 1) < 0.1) {
        position.velocity = [0, 0];
    }
    position.timestamp = timestamp;
    return position;
}

function caseSeeds(concrete_seed: number | undefined): number[] {
    if (concrete_seed !== undefined) {
        return [concrete_seed];
    }
    return Array.from(
        { length: CASES },
        () => 1 + Math.floor(Math.random() * 1_000_000),
    );
}

function expectHitsTarget(
    plan: FlightPlan,
    position: Position,
    target: Position,
    seed: number,
    label: string,
): void {
    const arrive = plan.apply_to(position);
    const predicted = predict_position(target, arrive.timestamp);
    expect(
        distance(arrive, predicted),
        `seed ${seed} ${label} position`,
    ).toBeLessThan(POSITION_DELTA);
    expect(
        vecAbs(vecSub(arrive.velocity, predicted.velocity)),
        `seed ${seed} ${label} velocity`,
    ).toBeLessThan(VELOCITY_DELTA);
}

test("plans intercepts for random start and target states", () => {
    const concrete_seed: number | undefined = undefined;

    // 1. generate a seed for each case
    const seeds = caseSeeds(concrete_seed);

    // 2. run each random intercept case
    for (const seed of seeds) {
        try {
            const rng = new Randomizer(seed);

            // 2.1 generate random start and target
            const timestamp = BigInt(rng.randomInt(0, 10_000_000));
            const position = randomKinematic(rng, timestamp);
            const target = randomKinematic(rng, timestamp);
            const amax = rng.randomValue(5, 100);

            // 2.2 compute an intercept plan
            const plan = approach_to_plan(position, target, amax);
            expect(plan, `seed ${seed}`).toBeDefined();
            if (!plan) {
                return;
            }

            // 2.3 check arrival matches the target
            expectHitsTarget(plan, position, target, seed, "intercept");
        } catch (error) {
            console.error(`failed seed: ${seed}`);
            throw error;
        }
    }
});

test("prepares a plan in a requested time window", () => {
    const concrete_seed: number | undefined = undefined;

    // 1. generate a seed for each case
    const seeds = caseSeeds(concrete_seed);

    // 2. run each random time-window case
    for (const seed of seeds) {
        try {
            const rng = new Randomizer(seed);

            // 2.1 generate random start and target
            const timestamp = BigInt(rng.randomInt(0, 10_000_000));
            const position = randomKinematic(rng, timestamp);
            const target = randomKinematic(rng, timestamp);
            const amax = rng.randomValue(5, 100);

            // 2.2 plan at full thrust
            const fastest = approach_to_plan(position, target, amax);
            expect(fastest, `seed ${seed}`).toBeDefined();
            if (!fastest) {
                return;
            }
            const t = fastest.duration_sec();
            if (t <= 0) {
                continue;
            }

            // 2.3 check small windows slower than full thrust
            for (let i = 0; i < INNER_WINDOWS; i += 1) {
                // 2.3.1 pick a window after t
                const tmin = t * rng.randomValue(1.2, 2.5);
                const tmax = tmin + t * rng.randomValue(0.05, 0.3);

                // 2.3.2 plan in that window
                const [status, plan] = prepare_flight_plan_in_time(
                    position,
                    target,
                    amax,
                    tmin,
                    tmax,
                );
                expect(status.is_ok(), `seed ${seed} inner ${i}`).toBe(true);
                expect(plan, `seed ${seed} inner ${i}`).toBeDefined();
                if (!plan) {
                    return;
                }

                // 2.3.3 check duration and intercept
                expect(plan.duration_sec(), `seed ${seed} inner ${i} tmin`)
                    .toBeGreaterThanOrEqual(tmin);
                expect(plan.duration_sec(), `seed ${seed} inner ${i} tmax`)
                    .toBeLessThanOrEqual(tmax);
                expectHitsTarget(plan, position, target, seed, `inner ${i}`);
            }

            // 2.4 check a window that contains full-thrust duration
            const [boundary_status, boundary_plan] = prepare_flight_plan_in_time(
                position,
                target,
                amax,
                t * rng.randomValue(0.3, 0.9),
                t * rng.randomValue(1.1, 2),
            );
            expect(boundary_status.is_ok(), `seed ${seed} boundary`).toBe(true);
            expect(boundary_plan, `seed ${seed} boundary`).toBeDefined();
            if (!boundary_plan) {
                return;
            }
            expect(boundary_plan.duration_sec(), `seed ${seed} boundary duration`)
                .toBeCloseTo(t, 6);
            expectHitsTarget(
                boundary_plan,
                position,
                target,
                seed,
                "boundary",
            );

            // 2.5 check a window shorter than full thrust
            const too_late_max = t * rng.randomValue(0.1, 0.9);
            const [late_status, late_plan] = prepare_flight_plan_in_time(
                position,
                target,
                amax,
                rng.randomValue(0, too_late_max),
                too_late_max,
            );
            expect(late_status.is_ok(), `seed ${seed} too late`).toBe(false);
            expect(late_plan, `seed ${seed} too late`).toBeUndefined();

            // 2.6 check a too-narrow window
            const tight = t * rng.randomValue(1.5, 3);
            const [tight_status, tight_plan] = prepare_flight_plan_in_time(
                position,
                target,
                amax,
                tight,
                tight,
            );
            expect(tight_status.is_ok(), `seed ${seed} tight`).toBe(false);
            expect(tight_plan, `seed ${seed} tight`).toBeUndefined();
        } catch (error) {
            console.error(`failed seed: ${seed}`);
            throw error;
        }
    }
});

test("prepares a plan in a requested delta-v window", () => {
    const concrete_seed: number | undefined = undefined;

    // 1. generate a seed for each case
    const seeds = caseSeeds(concrete_seed);

    // 2. run each random delta-v window case
    for (const seed of seeds) {
        try {
            const rng = new Randomizer(seed);

            // 2.1 generate random start and target
            const timestamp = BigInt(rng.randomInt(0, 10_000_000));
            const position = randomKinematic(rng, timestamp);
            const target = randomKinematic(rng, timestamp);
            const amax = rng.randomValue(5, 100);

            // 2.2 plan at full thrust and at amax/100
            const fastest = approach_to_plan(position, target, amax);
            expect(fastest, `seed ${seed}`).toBeDefined();
            if (!fastest) {
                return;
            }
            const slowest = approach_to_plan(position, target, amax / 100);
            expect(slowest, `seed ${seed} amax/100`).toBeDefined();
            if (!slowest) {
                return;
            }
            const dv = fastest.delta_v();
            const dv_low = slowest.delta_v();
            if (dv <= 0 || dv_low >= dv) {
                continue;
            }
            const span = dv - dv_low;

            // 2.3 check small windows between amax/100 and full-thrust delta-v
            for (let i = 0; i < INNER_WINDOWS; i += 1) {
                // 2.3.1 pick a window between the two delta-v values
                const dvmin = dv_low + span * rng.randomValue(0.15, 0.5);
                const dvmax = dvmin + span * rng.randomValue(0.1, 0.35);
                if (dvmax >= dv) {
                    continue;
                }

                // 2.3.2 plan in that window
                const [status, plan] = prepare_flight_plan_in_delta_v(
                    position,
                    target,
                    amax,
                    dvmin,
                    dvmax,
                );
                expect(status.is_ok(), `seed ${seed} inner ${i}`).toBe(true);
                expect(plan, `seed ${seed} inner ${i}`).toBeDefined();
                if (!plan) {
                    return;
                }

                // 2.3.3 check delta-v and intercept
                expect(plan.delta_v(), `seed ${seed} inner ${i} dvmin`)
                    .toBeGreaterThanOrEqual(dvmin);
                expect(plan.delta_v(), `seed ${seed} inner ${i} dvmax`)
                    .toBeLessThanOrEqual(dvmax);
                expectHitsTarget(plan, position, target, seed, `inner ${i}`);
            }

            // 2.4 check a window that contains full-thrust delta-v
            const [boundary_status, boundary_plan] = prepare_flight_plan_in_delta_v(
                position,
                target,
                amax,
                dv * rng.randomValue(0.3, 0.9),
                dv * rng.randomValue(1.1, 2),
            );
            expect(boundary_status.is_ok(), `seed ${seed} boundary`).toBe(true);
            expect(boundary_plan, `seed ${seed} boundary`).toBeDefined();
            if (!boundary_plan) {
                return;
            }
            expect(boundary_plan.delta_v(), `seed ${seed} boundary delta-v`)
                .toBeCloseTo(dv, 6);
            expectHitsTarget(
                boundary_plan,
                position,
                target,
                seed,
                "boundary",
            );

            // 2.5 check a window above full-thrust delta-v
            const too_high_min = dv * rng.randomValue(1.1, 2);
            const [high_status, high_plan] = prepare_flight_plan_in_delta_v(
                position,
                target,
                amax,
                too_high_min,
                too_high_min + dv * rng.randomValue(0.1, 0.5),
            );
            expect(high_status.is_ok(), `seed ${seed} too high`).toBe(false);
            expect(high_plan, `seed ${seed} too high`).toBeUndefined();

            // 2.6 check a too-narrow window
            const tight = dv_low + span * rng.randomValue(0.3, 0.7);
            const [tight_status, tight_plan] = prepare_flight_plan_in_delta_v(
                position,
                target,
                amax,
                tight,
                tight,
            );
            expect(tight_status.is_ok(), `seed ${seed} tight`).toBe(false);
            expect(tight_plan, `seed ${seed} tight`).toBeUndefined();
        } catch (error) {
            console.error(`failed seed: ${seed}`);
            throw error;
        }
    }
});
