import type { Point, Position, Vector } from "../types/common.js";
import { Status } from "../types/status.js";
import { predict_position } from "../utils/predictor.js";
import {
    almostNull,
    codirected,
    vecAbs,
    vecAdd,
    vecScale,
    vecSub,
} from "../utils/vector.js";
import type { Engine } from "./engine.js";
import type { Ship } from "./ship.js";

const IDLE = 1e-9;
const BURN_MIN_SEC = 1e-12;
const AXIS_TIME_MATCH_SEC = 1e-4;
const SPLIT_ITERATIONS = 40;
const SEARCH_CYCLES = 32;

type PlanPosition = {
    timestamp: bigint | undefined;
    point: Point;
    velocity: Vector;
};

type AxisBurn = {
    duration_sec: number;
    acc: number;
};

function as_plan(position: Position): PlanPosition {
    return {
        timestamp: position.timestamp,
        point: position.point,
        velocity: position.velocity,
    };
}

function usec(position: PlanPosition): bigint {
    return position.timestamp ?? 0n;
}

function predict(position: PlanPosition, at: bigint): PlanPosition {
    const predicted = predict_position(
        {
            timestamp: usec(position),
            point: position.point,
            velocity: position.velocity,
        },
        at,
    );
    return {
        timestamp: predicted.timestamp,
        point: predicted.point,
        velocity: predicted.velocity,
    };
}

function duration_us(seconds: number): bigint {
    return BigInt(Math.round(seconds * 1e6));
}

function accelerate_plan(
    start: PlanPosition,
    acc: Vector,
    t_sec: number,
): PlanPosition {
    const dv = vecScale(acc, t_sec);
    const ds = vecScale(vecAdd(start.velocity, vecScale(dv, 0.5)), t_sec);
    const end_at = t_sec * 1e6 + Number(usec(start));
    return {
        timestamp: BigInt(Math.round(end_at)),
        point: [start.point[0] + ds[0], start.point[1] + ds[1]],
        velocity: vecAdd(start.velocity, dv),
    };
}

export class Maneuver {
    constructor(
        readonly at: bigint,
        readonly duration: bigint,
        readonly acc: Vector,
    ) {}

    ends_at(): bigint {
        return this.at + this.duration;
    }

    apply_to(position: PlanPosition): PlanPosition {
        let current = position;
        if (current.timestamp !== undefined) {
            current = predict(current, this.at);
        }
        return accelerate_plan(current, this.acc, Number(this.duration) / 1e6);
    }

    partially_apply_to(position: PlanPosition, duration_usec: bigint): PlanPosition {
        return accelerate_plan(position, this.acc, Number(duration_usec) / 1e6);
    }
}

function squash_maneuvers(maneuvers: Maneuver[]): Maneuver[] {
    const result: Maneuver[] = [];
    for (const maneuver of maneuvers) {
        const previous = result.at(-1);
        const do_squash = previous !== undefined
            && codirected(previous.acc, maneuver.acc)
            && vecAbs(vecSub(previous.acc, maneuver.acc)) < 0.00001;
        if (do_squash && previous) {
            result[result.length - 1] = new Maneuver(
                previous.at,
                previous.duration + maneuver.duration,
                previous.acc,
            );
        } else {
            result.push(maneuver);
        }
    }
    return result;
}

export class FlightPlan {
    constructor(readonly maneuvers: Maneuver[]) {}

    time_points(): bigint[] {
        const points: bigint[] = [];
        for (const maneuver of this.maneuvers) {
            const last = points.at(-1);
            if (last === undefined || last < maneuver.at) {
                points.push(maneuver.at, maneuver.at + maneuver.duration);
            } else {
                points.push(maneuver.at + maneuver.duration);
            }
        }
        return points;
    }

    acceleration_at(at_us: bigint): Vector {
        for (const maneuver of this.maneuvers) {
            if (at_us < maneuver.at) {
                return [0, 0];
            }
            if (maneuver.at <= at_us && at_us < maneuver.at + maneuver.duration) {
                return maneuver.acc;
            }
        }
        return [0, 0];
    }

    max_acceleration(): number {
        let max = 0;
        for (const maneuver of this.maneuvers) {
            const magnitude = vecAbs(maneuver.acc);
            if (magnitude > max) {
                max = magnitude;
            }
        }
        return max;
    }

    duration_usec(): number {
        if (this.maneuvers.length === 0) {
            return 0;
        }
        return Number(this.ends_at() - this.starts_at());
    }

    duration_sec(): number {
        return this.duration_usec() / 1e6;
    }

    delta_v(): number {
        let total = 0;
        for (const maneuver of this.maneuvers) {
            total += vecAbs(maneuver.acc) * Number(maneuver.duration) / 1e6;
        }
        return total;
    }

    starts_at(): bigint {
        return this.maneuvers[0]?.at ?? 0n;
    }

    ends_at(): bigint {
        const last = this.maneuvers.at(-1);
        if (!last) {
            return 0n;
        }
        return last.at + last.duration;
    }

    static merge(plans: FlightPlan[], squash = true): FlightPlan {
        const time_points = [...new Set(plans.flatMap((plan) => plan.time_points()))]
            .sort((left, right) => (left < right ? -1 : left > right ? 1 : 0));
        if (time_points.length === 0) {
            return new FlightPlan([]);
        }

        const maneuvers: Maneuver[] = [];
        for (let i = 0; i < time_points.length - 1; i += 1) {
            const begin = time_points[i];
            const end = time_points[i + 1];
            if (begin === undefined || end === undefined) {
                continue;
            }
            let acceleration: Vector = [0, 0];
            for (const plan of plans) {
                acceleration = vecAdd(acceleration, plan.acceleration_at(begin));
            }
            maneuvers.push(new Maneuver(begin, end - begin, acceleration));
        }
        return new FlightPlan(squash ? squash_maneuvers(maneuvers) : maneuvers);
    }

    apply_to(position: Position): Position {
        let current = as_plan(position);
        for (const maneuver of this.maneuvers) {
            current = maneuver.apply_to(current);
        }
        return {
            timestamp: usec(current),
            point: current.point,
            velocity: current.velocity,
        };
    }

    apply_to_plan(position: PlanPosition): PlanPosition {
        let current = position;
        for (const maneuver of this.maneuvers) {
            current = maneuver.apply_to(current);
        }
        return current;
    }

    partially_apply_to(position: PlanPosition, duration_usec: bigint): PlanPosition {
        let current = position;
        let remaining = duration_usec;
        for (const maneuver of this.maneuvers) {
            if (usec(current) < maneuver.ends_at()) {
                const dt = min_bigint(maneuver.ends_at() - usec(current), remaining);
                current = maneuver.partially_apply_to(current, dt);
                remaining -= dt;
                if (remaining <= 0n) {
                    break;
                }
            }
        }
        return current;
    }
}

function min_bigint(left: bigint, right: bigint): bigint {
    return left < right ? left : right;
}

function sgn(value: number): number {
    if (value < 0) {
        return -1;
    }
    if (value > 0) {
        return 1;
    }
    return 0;
}

function axis_idle(x: number, v: number): boolean {
    return Math.abs(x) <= IDLE && Math.abs(v) <= IDLE;
}

function burns_duration(burns: AxisBurn[]): number {
    return burns.reduce((sum, burn) => sum + burn.duration_sec, 0);
}

function two_bang(x: number, v: number, a1: number): AxisBurn[] | undefined {
    const delta = 0.5 * (v / a1) ** 2 - x / a1;
    if (!(delta >= 0)) {
        return undefined;
    }
    const t2 = Math.sqrt(delta);
    const t1 = t2 - v / a1;
    if (t1 < -BURN_MIN_SEC || t2 < -BURN_MIN_SEC) {
        return undefined;
    }
    const burns: AxisBurn[] = [];
    if (t1 > BURN_MIN_SEC) {
        burns.push({ duration_sec: t1, acc: a1 });
    }
    if (t2 > BURN_MIN_SEC) {
        burns.push({ duration_sec: t2, acc: -a1 });
    }
    return burns;
}

// 1D: from (x, v) to (0, 0) with |acc| = amax.
// One burn if braking already stops on the origin; otherwise accel then brake.
function stop_at_zero_1d(
    x: number,
    v: number,
    amax: number,
): AxisBurn[] | undefined {
    if (axis_idle(x, v)) {
        return [];
    }
    if (!(amax > 0) || !Number.isFinite(amax)) {
        return undefined;
    }

    const stop_offset = v * Math.abs(v) / (2 * amax);
    if (Math.abs(x + stop_offset) <= IDLE) {
        const t = Math.abs(v) / amax;
        if (t <= BURN_MIN_SEC) {
            return [];
        }
        return [{ duration_sec: t, acc: -sgn(v) * amax }];
    }

    let best: AxisBurn[] | undefined;
    let best_time = Infinity;
    for (const acc of [amax, -amax]) {
        const burns = two_bang(x, v, acc);
        if (!burns || burns.length === 0) {
            continue;
        }
        const time = burns_duration(burns);
        if (time < best_time) {
            best_time = time;
            best = burns;
        }
    }
    return best;
}

function axis_plan(
    burns: AxisBurn[],
    axis: 0 | 1,
    now: bigint,
): FlightPlan {
    const maneuvers: Maneuver[] = [];
    let at = now;
    for (const burn of burns) {
        const duration = duration_us(burn.duration_sec);
        if (duration <= 0n) {
            continue;
        }
        const acc: Vector = [0, 0];
        acc[axis] = burn.acc;
        maneuvers.push(new Maneuver(at, duration, acc));
        at += duration;
    }
    return new FlightPlan(maneuvers);
}

function split_amax(
    x: number,
    vx: number,
    y: number,
    vy: number,
    amax: number,
): [number, number] | undefined {
    const x_idle = axis_idle(x, vx);
    const y_idle = axis_idle(y, vy);
    if (x_idle && y_idle) {
        return [amax, 0];
    }
    if (x_idle) {
        return [0, amax];
    }
    if (y_idle) {
        return [amax, 0];
    }

    let left = 0;
    let right = Math.PI / 2;
    let best: [number, number] | undefined;
    let best_error = Infinity;
    for (let i = 0; i < SPLIT_ITERATIONS; i += 1) {
        const alfa = (left + right) / 2;
        const ax = amax * Math.cos(alfa);
        const ay = amax * Math.sin(alfa);
        const x_burns = stop_at_zero_1d(x, vx, ax);
        const y_burns = stop_at_zero_1d(y, vy, ay);
        if (!x_burns || !y_burns) {
            return undefined;
        }
        const error = burns_duration(x_burns) - burns_duration(y_burns);
        if (Math.abs(error) < best_error) {
            best_error = Math.abs(error);
            best = [ax, ay];
        }
        if (Math.abs(error) < AXIS_TIME_MATCH_SEC) {
            return [ax, ay];
        }
        if (error < 0) {
            left = alfa;
        } else {
            right = alfa;
        }
    }
    return best;
}

function stop_at_origin(
    position: PlanPosition,
    amax: number,
): FlightPlan | undefined {
    if (!(amax > 0) || !Number.isFinite(amax)) {
        return almostNull(position.point) && almostNull(position.velocity)
            ? new FlightPlan([])
            : undefined;
    }

    const [x, y] = position.point;
    const [vx, vy] = position.velocity;
    const split = split_amax(x, vx, y, vy, amax);
    if (!split) {
        return undefined;
    }
    const [ax, ay] = split;
    const now = usec(position);
    const x_burns = stop_at_zero_1d(x, vx, ax);
    const y_burns = stop_at_zero_1d(y, vy, ay);
    if (!x_burns || !y_burns) {
        return undefined;
    }
    return FlightPlan.merge([
        axis_plan(x_burns, 0, now),
        axis_plan(y_burns, 1, now),
    ]);
}

function relative_to_target(
    position: PlanPosition,
    target: PlanPosition,
): PlanPosition {
    const target_now = target.timestamp === undefined
        ? target
        : predict(target, usec(position));
    return {
        timestamp: position.timestamp,
        point: vecSub(position.point, target_now.point),
        velocity: vecSub(position.velocity, target_now.velocity),
    };
}

function plan_to_target(
    position: Position,
    target: Position,
    amax: number,
): FlightPlan | undefined {
    return stop_at_origin(
        relative_to_target(as_plan(position), as_plan(target)),
        amax,
    );
}

export function prepare_flight_plan(
    position: Position,
    target: Position,
    amax: number,
): FlightPlan | undefined {
    return plan_to_target(position, target, amax);
}

export function approach_to_plan(
    position: Position,
    target: Position,
    amax: number,
): FlightPlan | undefined {
    return plan_to_target(position, target, amax);
}

export function prepare_flight_plan_in_time(
    position: Position,
    target: Position,
    amax: number,
    tmin: number,
    tmax: number,
): [Status, FlightPlan | undefined] {
    const fastest = plan_to_target(position, target, amax);
    if (!fastest) {
        return [Status.fail("failed to build a plan"), undefined];
    }

    const fastest_t = fastest.duration_sec();
    if (fastest_t > tmax) {
        return [Status.fail("full thrust exceeds tmax"), undefined];
    }
    if (fastest_t >= tmin) {
        return [Status.ok(), fastest];
    }

    let left = 0;
    let right = amax;
    for (let cycle = 0; cycle < SEARCH_CYCLES; cycle += 1) {
        const candidate = (left + right) / 2;
        const plan = plan_to_target(position, target, candidate);
        if (!plan) {
            left = candidate;
            continue;
        }
        const duration = plan.duration_sec();
        if (duration >= tmin && duration <= tmax) {
            return [Status.ok(), plan];
        }
        if (duration < tmin) {
            right = candidate;
        } else {
            left = candidate;
        }
    }
    return [Status.fail("time window search did not converge"), undefined];
}

export function prepare_flight_plan_in_delta_v(
    position: Position,
    target: Position,
    amax: number,
    dvmin: number,
    dvmax: number,
): [Status, FlightPlan | undefined] {
    const fastest = plan_to_target(position, target, amax);
    if (!fastest) {
        return [Status.fail("failed to build a plan"), undefined];
    }

    const fastest_dv = fastest.delta_v();
    if (fastest_dv < dvmin) {
        return [Status.fail("full thrust below dvmin"), undefined];
    }
    if (fastest_dv <= dvmax) {
        return [Status.ok(), fastest];
    }

    let left = 0;
    let right = amax;
    for (let cycle = 0; cycle < SEARCH_CYCLES; cycle += 1) {
        const candidate = (left + right) / 2;
        const plan = plan_to_target(position, target, candidate);
        if (!plan) {
            left = candidate;
            continue;
        }
        const delta_v = plan.delta_v();
        if (delta_v >= dvmin && delta_v <= dvmax) {
            return [Status.ok(), plan];
        }
        if (delta_v > dvmax) {
            right = candidate;
        } else {
            left = candidate;
        }
    }
    return [Status.fail("delta-v window search did not converge"), undefined];
}

export type FlightClock = {
    wait_until(
        time_us: bigint,
        timeout_ms?: number,
    ): Promise<[Status, bigint | undefined]>;
};

export async function follow_flight_plan(
    ship: Ship,
    engine: Engine,
    plan: FlightPlan,
    clock: FlightClock,
): Promise<Status> {
    for (const maneuver of plan.maneuvers) {
        await clock.wait_until(maneuver.at - 25_000n);

        const [state_status, ship_state] = await ship.get_state();
        if (!state_status.is_ok() || ship_state === undefined
            || ship_state.weight === undefined)
        {
            return state_status.wrap("failed to get ship state");
        }

        const thrust = Math.round(ship_state.weight * vecAbs(maneuver.acc));
        const status = await engine.set_thrust(
            maneuver.acc[0],
            maneuver.acc[1],
            thrust,
            Math.round(Number(maneuver.duration) / 1000),
            maneuver.at,
        );
        if (!status.is_ok()) {
            return status.wrap("failed to set thrust");
        }
    }

    if (plan.maneuvers.length > 0) {
        await clock.wait_until(plan.ends_at());
    }
    return Status.ok();
}
