import type { Position, Vector } from "#sdk/types/common.js";
import { predict_position } from "./predictor.js";


export type Ship = {
    mass: number;
    position: Position;
    // orientation is a vector from ship's center to its front
    orientation: Vector;
    engine_max_thrust: number;
    max_rotate_speed: number; // max ships rotate speed in radians per second
}

export type Maneuver = {
    // Rotate ship to a given orientation in `duration` milliseconds
    kind: "rotate";
    target: Vector;
    duration_ms: number;
} | {
    // Turn on engine to a given thrust for next `duration` milliseconds.
    // Thrust is a magnitude; its direction is the ship's orientation.
    kind: "thrust";
    thrust: number;
    duration_ms: number;
};

export type FlightPlan = {
    maneuvers: {
        start_at: number;
        maneuver: Maneuver;
    }[];
}

// position.timestamp and start_at share one clock, in microseconds.
const US_PER_MS = 1000;
const ANGLE_EPSILON = 1e-9;

type ScheduledManeuver = FlightPlan["maneuvers"][number];

function maneuver_end(item: ScheduledManeuver): number {
    return item.start_at + item.maneuver.duration_ms * US_PER_MS;
}

// Shortest signed turn from `from` to `to`, in radians.
// Undefined when either vector has no direction.
function shortest_angle(from: Vector, to: Vector): number | undefined {
    if (Math.hypot(from[0], from[1]) === 0 || Math.hypot(to[0], to[1]) === 0) {
        return undefined;
    }
    let delta = Math.atan2(to[1], to[0]) - Math.atan2(from[1], from[0]);
    if (delta > Math.PI) {
        delta -= 2 * Math.PI;
    } else if (delta < -Math.PI) {
        delta += 2 * Math.PI;
    }
    return delta;
}

function turn_fits(
    from: Vector,
    to: Vector,
    duration_ms: number,
    max_rotate_speed: number,
): boolean {
    const angle = shortest_angle(from, to);
    if (angle === undefined || !(max_rotate_speed >= 0)) {
        return false;
    }
    const max_angle = max_rotate_speed * (duration_ms / 1000);
    return Math.abs(angle) <= max_angle + ANGLE_EPSILON;
}

function sorted_maneuvers(plan: FlightPlan): ScheduledManeuver[] {
    return [...plan.maneuvers].sort((left, right) => left.start_at - right.start_at);
}

// `maneuvers` are sorted by start_at.
function check_no_overlaps(maneuvers: readonly ScheduledManeuver[]): boolean {
    const overlap_tolerance_us = 10 * US_PER_MS;
    for (let i = 0; i < maneuvers.length - 1; i += 1) {
        const current = maneuvers[i];
        const next = maneuvers[i + 1];
        if (current === undefined || next === undefined) {
            continue;
        }
        if (next.start_at < maneuver_end(current) - overlap_tolerance_us) {
            return false;
        }
    }
    return true;
}

function schedule_is_finite(plan: FlightPlan): boolean {
    for (const item of plan.maneuvers) {
        if (!Number.isFinite(item.start_at) || !Number.isFinite(item.maneuver.duration_ms)) {
            return false;
        }
        if (item.maneuver.duration_ms < 0) {
            return false;
        }
        if (item.maneuver.kind === "thrust" && !Number.isFinite(item.maneuver.thrust)) {
            return false;
        }
    }
    return true;
}

// `plan.maneuvers` are sorted by start_at.
// At most one maneuver is active at a time: either thrust or rotation.
// A rotate must finish within its duration at max_rotate_speed.
// Thrust is a magnitude and cannot exceed engine_max_thrust.
export function verify_flight_plan(ship: Ship, plan: FlightPlan): boolean {
    if (!Number.isFinite(ship.engine_max_thrust)
        || !Number.isFinite(ship.max_rotate_speed)
        || !schedule_is_finite(plan))
    {
        return false;
    }

    if (!check_no_overlaps(plan.maneuvers)) {
        return false;
    }

    let orientation: Vector = [ship.orientation[0], ship.orientation[1]];
    for (const item of plan.maneuvers) {
        if (item.maneuver.kind === "thrust") {
            if (item.maneuver.thrust < 0 || item.maneuver.thrust > ship.engine_max_thrust) {
                return false;
            }
            continue;
        }

        if (!turn_fits(
            orientation,
            item.maneuver.target,
            item.maneuver.duration_ms,
            ship.max_rotate_speed,
        )) {
            return false;
        }
        orientation = [item.maneuver.target[0], item.maneuver.target[1]];
    }
    return true;
}

function clone_ship(ship: Ship): Ship {
    return {
        mass: ship.mass,
        position: {
            timestamp: ship.position.timestamp,
            point: [ship.position.point[0], ship.position.point[1]],
            velocity: [ship.position.velocity[0], ship.position.velocity[1]],
        },
        orientation: [ship.orientation[0], ship.orientation[1]],
        engine_max_thrust: ship.engine_max_thrust,
        max_rotate_speed: ship.max_rotate_speed,
    };
}

function coast_to(ship: Ship, at_us: number): void {
    const dt_sec = (at_us - ship.position.timestamp) / 1e6;
    ship.position = {
        timestamp: at_us,
        point: [
            ship.position.point[0] + ship.position.velocity[0] * dt_sec,
            ship.position.point[1] + ship.position.velocity[1] * dt_sec,
        ],
        velocity: [ship.position.velocity[0], ship.position.velocity[1]],
    };
}

// Thrust acceleration follows the ship's current orientation.
function thrust_to(ship: Ship, thrust: number, at_us: number): void {
    const dt_sec = (at_us - ship.position.timestamp) / 1e6;
    const length = Math.hypot(ship.orientation[0], ship.orientation[1]);
    if (thrust !== 0 && (!(ship.mass > 0) || length === 0)) {
        throw new Error("cannot apply thrust without mass and orientation");
    }
    const accel = thrust === 0 || length === 0 ? 0 : thrust / ship.mass;
    const ax = (ship.orientation[0] / (length || 1)) * accel;
    const ay = (ship.orientation[1] / (length || 1)) * accel;
    const vx = ship.position.velocity[0];
    const vy = ship.position.velocity[1];
    ship.position = {
        timestamp: at_us,
        point: [
            ship.position.point[0] + vx * dt_sec + 0.5 * ax * dt_sec * dt_sec,
            ship.position.point[1] + vy * dt_sec + 0.5 * ay * dt_sec * dt_sec,
        ],
        velocity: [vx + ax * dt_sec, vy + ay * dt_sec],
    };
}

// Applies flight plan to a ship and returns the final state of the ship.
// Until the first maneuver, and in every gap, the ship coasts.
export function replay_flight_plan(ship: Ship, flight_plan: FlightPlan)
: Ship
{
    const maneuvers = sorted_maneuvers(flight_plan);
    const first = maneuvers[0];
    if (first !== undefined && ship.position.timestamp > first.start_at) {
        throw new Error("ship time is past the first maneuver");
    }
    if (!verify_flight_plan(ship, { maneuvers })) {
        throw new Error("flight plan cannot be executed");
    }

    const result = clone_ship(ship);
    for (const item of maneuvers) {
        const end = maneuver_end(item);
        if (item.start_at > result.position.timestamp) {
            coast_to(result, item.start_at);
        }
        if (item.maneuver.kind === "rotate") {
            coast_to(result, end);
            result.orientation = [item.maneuver.target[0], item.maneuver.target[1]];
        } else {
            thrust_to(result, item.maneuver.thrust, end);
        }
    }
    return result;
}

const POSITION_TOLERANCE = 10;
const VELOCITY_TOLERANCE = 0.1;

type Trajectory = {
    // Velocities relative to the target, between full-thrust burns.
    via: Vector[];
    time: number;
    error: Vector;
};

type FlightProblem = {
    velocity: Vector;
    final_velocity: Vector;
    displacement: Vector;
    orientation: Vector;
    turn_time: number;
};

const magnitude = (v: Vector): number => Math.hypot(v[0], v[1]);
const scale = (v: Vector, k: number): Vector => [v[0] * k, v[1] * k];
const subtract = (a: Vector, b: Vector): Vector => [a[0] - b[0], a[1] - b[1]];
const dot = (a: Vector, b: Vector): number => a[0] * b[0] + a[1] * b[1];
const cross = (a: Vector, b: Vector): number => a[0] * b[1] - a[1] * b[0];

// First entry of a ballistic trajectory into an arrival circle.
function coast_time(displacement: Vector, velocity: Vector, radius: number): number {
    const outside = dot(displacement, displacement) - radius * radius;
    if (outside <= 0) {
        return 0;
    }
    const speed2 = dot(velocity, velocity);
    const towards = dot(displacement, velocity);
    const discriminant = speed2 * radius * radius - cross(displacement, velocity) ** 2;
    if (speed2 === 0 || towards <= 0 || discriminant < 0) {
        return Infinity;
    }
    return outside / (towards + Math.sqrt(discriminant));
}

type SingleBurn = {
    heading: Vector;
    turn: number;
    burn: number;
    before: number;
    after: number;
    time: number;
};

// For a given terminal velocity, choose ballistic waits before/after one burn.
// In the non-collinear case this is a linear objective over an ellipse in the
// two wait times. Also check each axis, where one wait is zero.
function one_burn(
    ship: Ship,
    displacement: Vector,
    velocity: Vector,
    final_velocity: Vector,
): SingleBurn | undefined {
    const dv = subtract(final_velocity, velocity);
    const speed = magnitude(dv);
    if (speed === 0) {
        return undefined;
    }
    const angle = Math.abs(shortest_angle(ship.orientation, dv)!);
    const turn = angle === 0 ? 0 : angle / ship.max_rotate_speed;
    const burn = speed * ship.mass / ship.engine_max_thrust;
    if (!Number.isFinite(turn + burn)) {
        return undefined;
    }
    const remaining: Vector = [
        displacement[0] - velocity[0] * (turn + burn / 2) - final_velocity[0] * burn / 2,
        displacement[1] - velocity[1] * (turn + burn / 2) - final_velocity[1] * burn / 2,
    ];
    const radius = POSITION_TOLERANCE - 0.001;
    const waits: Vector[] = [
        [coast_time(remaining, velocity, radius), 0],
        [0, coast_time(remaining, final_velocity, radius)],
    ];
    const determinant = cross(velocity, final_velocity);
    if (Math.abs(determinant) > 1e-12 * magnitude(velocity) * magnitude(final_velocity)) {
        const before = cross(remaining, final_velocity) / determinant;
        const after = cross(velocity, remaining) / determinant;
        // Gradient of total wait with respect to displacement, B^(-T) * [1,1].
        const gradient: Vector = [(final_velocity[1] - velocity[1]) / determinant,
            (velocity[0] - final_velocity[0]) / determinant];
        const offset = scale(gradient, -radius / magnitude(gradient));
        waits.push([before + cross(offset, final_velocity) / determinant,
            after + cross(velocity, offset) / determinant]);
    }
    let best: SingleBurn | undefined;
    for (const [before, after] of waits) {
        const time = before + turn + burn + after;
        if (before >= 0 && after >= 0 && Number.isFinite(time)
            && (!best || time < best.time))
        {
            best = { heading: scale(dv, 1 / speed), turn, burn, before, after, time };
        }
    }
    return best;
}

function simple_plan(ship: Ship, displacement: Vector, velocity: Vector): FlightPlan | undefined {
    let result: FlightPlan | undefined;
    let best_time = Infinity;
    if (magnitude(velocity) <= VELOCITY_TOLERANCE) {
        best_time = coast_time(displacement, velocity, POSITION_TOLERANCE - 0.001);
        if (Number.isFinite(best_time)) {
            result = { maneuvers: [{ start_at: ship.position.timestamp,
                maneuver: { kind: "thrust", thrust: 0, duration_ms: best_time * 1000 } }] };
        }
    }
    if (ship.engine_max_thrust === 0) {
        return result;
    }
    const speed = VELOCITY_TOLERANCE - 1e-6;
    const velocities: Vector[] = [[0, 0]];
    // Aim the slow final coast at the point remaining after braking. Iterate
    // because the small terminal velocity also affects the burn and turn.
    let final_velocity: Vector = [0, 0];
    for (let i = 0; i < 8; i += 1) {
        const dv = subtract(final_velocity, velocity);
        const angle = magnitude(dv) === 0 ? 0 : Math.abs(shortest_angle(ship.orientation, dv)!);
        const turn = angle === 0 ? 0 : angle / ship.max_rotate_speed;
        const burn = magnitude(dv) * ship.mass / ship.engine_max_thrust;
        if (!Number.isFinite(turn + burn)) {
            break;
        }
        const remaining: Vector = [
            displacement[0] - velocity[0] * (turn + burn / 2) - final_velocity[0] * burn / 2,
            displacement[1] - velocity[1] * (turn + burn / 2) - final_velocity[1] * burn / 2,
        ];
        if (magnitude(remaining) === 0) {
            break;
        }
        final_velocity = scale(remaining, speed / magnitude(remaining));
        velocities.push(final_velocity);
    }
    for (let i = 0; i < 16; i += 1) {
        const angle = i * Math.PI / 8;
        velocities.push([speed * Math.cos(angle), speed * Math.sin(angle)]);
    }
    for (const final of velocities) {
        const candidate = one_burn(ship, displacement, velocity, final);
        if (!candidate || candidate.time >= best_time) {
            continue;
        }
        best_time = candidate.time;
        result = { maneuvers: [] };
        let at = ship.position.timestamp + candidate.before * 1e6;
        if (candidate.turn > 0) {
            result.maneuvers.push({ start_at: at, maneuver: { kind: "rotate",
                target: candidate.heading, duration_ms: candidate.turn * 1000 } });
            at += candidate.turn * 1e6;
        }
        result.maneuvers.push({ start_at: at, maneuver: { kind: "thrust",
            thrust: ship.engine_max_thrust, duration_ms: candidate.burn * 1000 } });
        at += candidate.burn * 1e6;
        if (candidate.after > 0) {
            result.maneuvers.push({ start_at: at, maneuver: { kind: "thrust",
                thrust: 0, duration_ms: candidate.after * 1000 } });
        }
    }
    return result;
}

// Units are chosen so maximum acceleration is one. A velocity polygon defines
// the burns completely: dv = next - previous, duration = |dv|. Rotation is
// ballistic, so its displacement is previous * angle / angular_speed.
function evaluate(problem: FlightProblem, via: Vector[]): Trajectory {
    let velocity = problem.velocity;
    let heading = problem.orientation;
    let time = 0;
    const point: Vector = [0, 0];
    for (const next of [...via, problem.final_velocity]) {
        const dv = subtract(next, velocity);
        const burn = magnitude(dv);
        if (burn === 0) {
            continue;
        }
        const turn = Math.abs(shortest_angle(heading, dv)!) * problem.turn_time;
        for (const axis of [0, 1] as const) {
            point[axis] += velocity[axis] * turn
                + (velocity[axis] + next[axis]) * burn / 2;
        }
        time += turn + burn;
        velocity = next;
        heading = dv;
    }
    return { via, time, error: subtract(point, problem.displacement) };
}

type Derivative = { position: Vector; time: number };

function perturb(via: Vector[], coordinate: number, step: number): Vector[] {
    return via.map((v, i) => [
        v[0] + (2 * i === coordinate ? step : 0),
        v[1] + (2 * i + 1 === coordinate ? step : 0),
    ]);
}

function derivatives(problem: FlightProblem, via: Vector[]): Derivative[] {
    const step = 1e-6;
    return Array.from({ length: 2 * via.length }, (_, coordinate) => {
        const plus = evaluate(problem, perturb(via, coordinate, step));
        const minus = evaluate(problem, perturb(via, coordinate, -step));
        return {
            position: scale(subtract(plus.error, minus.error), 1 / (2 * step)),
            time: (plus.time - minus.time) / (2 * step),
        };
    });
}

// Solve (J J^T) multiplier = rhs. There are only two position constraints;
// the final velocity is fixed, for any intermediate velocities.
function multipliers(jacobian: Derivative[], rhs: Vector): Vector | undefined {
    let xx = 0;
    let xy = 0;
    let yy = 0;
    for (const { position: [x, y] } of jacobian) {
        xx += x * x;
        xy += x * y;
        yy += y * y;
    }
    const determinant = xx * yy - xy * xy;
    if (!(determinant > 1e-14 * xx * yy)) {
        return undefined;
    }
    return [(yy * rhs[0] - xy * rhs[1]) / determinant,
        (xx * rhs[1] - xy * rhs[0]) / determinant];
}

function step_via(via: Vector[], direction: number[], step: number): Vector[] {
    return via.map((v, i) => [
        v[0] + step * direction[2 * i]!,
        v[1] + step * direction[2 * i + 1]!,
    ]);
}

// Damped Newton projection onto the two position constraints. Return only
// converged trajectories: failure must never become an inaccurate flight plan.
function meet_target(
    problem: FlightProblem,
    via: Vector[],
    tolerance: number,
): Trajectory | undefined {
    let current = evaluate(problem, via);
    for (let iteration = 0; iteration < 32; iteration += 1) {
        const error = magnitude(current.error);
        if (error <= tolerance) {
            return current;
        }
        const jacobian = derivatives(problem, current.via);
        const multiplier = multipliers(jacobian, current.error);
        if (!multiplier) {
            return undefined;
        }
        const direction = jacobian.map(({ position: [x, y] }) =>
            -x * multiplier[0] - y * multiplier[1]);
        let next: Trajectory | undefined;
        for (let fraction = 1; fraction >= 1 / 1024; fraction /= 2) {
            const trial = evaluate(problem, step_via(current.via, direction, fraction));
            if (magnitude(trial.error) < error) {
                next = trial;
                break;
            }
        }
        if (!next) {
            return undefined;
        }
        current = next;
    }
    return magnitude(current.error) <= tolerance ? current : undefined;
}

// Descend along the position-constraint surface. Each accepted step is projected
// back onto that surface and must decrease elapsed time, including all turns.
function shorten(
    problem: FlightProblem,
    initial: Trajectory,
    tolerance: number,
): Trajectory {
    let current = initial;
    for (let iteration = 0; iteration < 48; iteration += 1) {
        const jacobian = derivatives(problem, current.via);
        const rhs: Vector = [0, 0];
        for (const { position, time } of jacobian) {
            rhs[0] += position[0] * time;
            rhs[1] += position[1] * time;
        }
        const multiplier = multipliers(jacobian, rhs);
        if (!multiplier) {
            break;
        }
        const direction = jacobian.map(({ position: [x, y], time }) =>
            x * multiplier[0] + y * multiplier[1] - time);
        const norm = Math.hypot(...direction);
        if (norm < 1e-7) {
            break;
        }
        let next: Trajectory | undefined;
        for (let step = Math.min(0.25 / norm, 1); step * norm > 1e-7; step /= 2) {
            const trial = meet_target(problem, step_via(current.via, direction, step), tolerance);
            if (trial && trial.time < current.time - 1e-9) {
                next = trial;
                break;
            }
        }
        if (!next) {
            break;
        }
        current = next;
    }
    return current;
}

// A constructive feasible seed: cancel relative velocity, then do a straight
// rest-to-rest transfer. The 180-degree turn contributes peak_speed * turn_time
// to distance. The stable quadratic formula also works for very short trips.
function safe_seed(problem: FlightProblem): Trajectory {
    const speed = magnitude(problem.velocity);
    const brake_turn = speed === 0 ? 0
        : Math.abs(shortest_angle(problem.orientation, scale(problem.velocity, -1))!)
            * problem.turn_time;
    const remaining = subtract(problem.displacement,
        scale(problem.velocity, brake_turn + speed / 2));
    const distance = magnitude(remaining);
    const flip = Math.PI * problem.turn_time;
    const peak = 2 * distance / (Math.sqrt(flip * flip + 4 * distance) + flip);
    const via: Vector[] = speed === 0 ? [] : [[0, 0]];
    if (distance > 0) {
        via.push(scale(remaining, peak / distance));
    }
    return evaluate(problem, via);
}

function schedule(ship: Ship, via: Vector[], target_velocity: Vector, final_velocity: Vector): FlightPlan {
    const plan: FlightPlan = { maneuvers: [] };
    let at = ship.position.timestamp;
    let heading = ship.orientation;
    let velocity = subtract(ship.position.velocity, target_velocity);
    const acceleration = ship.engine_max_thrust / ship.mass;
    const append = (maneuver: Maneuver): void => {
        plan.maneuvers.push({ start_at: at, maneuver });
        at += maneuver.duration_ms * US_PER_MS;
    };
    for (const next of [...via, final_velocity]) {
        const dv = subtract(next, velocity);
        const speed = magnitude(dv);
        if (speed === 0) {
            continue;
        }
        const angle = Math.abs(shortest_angle(heading, dv)!);
        if (angle > 0) {
            append({ kind: "rotate", target: scale(dv, 1 / speed),
                duration_ms: angle / ship.max_rotate_speed * 1000 });
        }
        append({ kind: "thrust", thrust: ship.engine_max_thrust,
            duration_ms: speed / acceleration * 1000 });
        heading = dv;
        velocity = next;
    }
    return plan;
}

// Use the arrival tolerances in the direction of decreasing flight time.
// The constraint multipliers give d(time)/d(destination). For final velocity,
// subtract the position correction from the direct change in elapsed time.
function relax_arrival(
    problem: FlightProblem,
    trajectory: Trajectory,
    position_radius: number,
    speed_radius: number,
    tolerance: number,
): { problem: FlightProblem; trajectory: Trajectory } {
    let current = { problem, trajectory };
    for (let iteration = 0; iteration < 4; iteration += 1) {
        const jacobian = derivatives(current.problem, current.trajectory.via);
        const rhs: Vector = [0, 0];
        for (const { position, time } of jacobian) {
            rhs[0] += position[0] * time;
            rhs[1] += position[1] * time;
        }
        const multiplier = multipliers(jacobian, rhs);
        if (!multiplier || magnitude(multiplier) === 0) {
            break;
        }
        const gradient: Vector = [0, 0];
        for (const axis of [0, 1] as const) {
            const step = 1e-6;
            const plus_velocity: Vector = [...current.problem.final_velocity];
            const minus_velocity: Vector = [...current.problem.final_velocity];
            plus_velocity[axis] += step;
            minus_velocity[axis] -= step;
            const plus = evaluate({ ...current.problem, final_velocity: plus_velocity }, current.trajectory.via);
            const minus = evaluate({ ...current.problem, final_velocity: minus_velocity }, current.trajectory.via);
            const dp = subtract(plus.error, minus.error);
            gradient[axis] = (plus.time - minus.time
                - multiplier[0] * dp[0] - multiplier[1] * dp[1]) / (2 * step);
        }
        const next_problem: FlightProblem = {
            ...problem,
            displacement: subtract(problem.displacement,
                scale(multiplier, position_radius / magnitude(multiplier))),
            final_velocity: magnitude(gradient) === 0 ? [0, 0]
                : scale(gradient, -speed_radius / magnitude(gradient)),
        };
        const candidate = meet_target(next_problem, current.trajectory.via, tolerance);
        if (!candidate || candidate.time >= current.trajectory.time) {
            break;
        }
        current = { problem: next_problem, trajectory: candidate };
    }
    return current;
}

function reaches_target(ship: Ship, target: Position, plan: FlightPlan): boolean {
    if (!verify_flight_plan(ship, plan)) {
        return false;
    }
    const arrived = replay_flight_plan(ship, plan).position;
    const predicted = predict_position(target, arrived.timestamp);
    return magnitude(subtract(arrived.point, predicted.point)) <= POSITION_TOLERANCE
        && magnitude(subtract(arrived.velocity, target.velocity)) <= VELOCITY_TOLERANCE;
}

/**
 * Intercept a target moving at constant velocity, with sequential rotation and
 * thrust. Times are continuous (fractional milliseconds are allowed).
 *
 * Searches full-thrust velocity polygons with up to four burns, minimizing
 * elapsed time including rotation. This is a numerical local optimization,
 * not a proof of the global optimum over arbitrary numbers of maneuvers.
 * A constructive feasible trajectory is retained if numerical search fails.
 * Also compares single burns with ballistic waits. General transfers require
 * positive mass, thrust and rotation speed; ballistic and single-burn arrivals
 * may still be possible without rotation or thrust.
 */
export function build_plan(ship: Ship, target: Position): FlightPlan {
    const values = [ship.mass, ship.engine_max_thrust, ship.max_rotate_speed,
        ...ship.orientation, ship.position.timestamp, ...ship.position.point,
        ...ship.position.velocity, target.timestamp, ...target.point, ...target.velocity];
    if (!values.every(Number.isFinite) || ship.mass <= 0
        || ship.engine_max_thrust < 0 || ship.max_rotate_speed < 0
        || magnitude(ship.orientation) === 0)
    {
        throw new RangeError("invalid ship or target kinematics");
    }

    const displacement = subtract(predict_position(target, ship.position.timestamp).point,
        ship.position.point);
    const velocity = subtract(ship.position.velocity, target.velocity);
    if (magnitude(displacement) <= POSITION_TOLERANCE
        && magnitude(velocity) <= VELOCITY_TOLERANCE)
    {
        return { maneuvers: [] };
    }
    const simple = simple_plan(ship, displacement, velocity);
    if (ship.engine_max_thrust === 0 || ship.max_rotate_speed === 0) {
        if (simple && reaches_target(ship, target, simple)) {
            return simple;
        }
        throw new RangeError("no arrival found without thrust or rotation");
    }

    const acceleration = ship.engine_max_thrust / ship.mass;
    const velocity_unit = Math.max(magnitude(velocity),
        Math.sqrt(acceleration * magnitude(displacement)),
        acceleration / ship.max_rotate_speed);
    const time_unit = velocity_unit / acceleration;
    const length_unit = velocity_unit * time_unit;
    if (![velocity_unit, time_unit, length_unit].every(value => Number.isFinite(value) && value > 0)) {
        throw new RangeError("flight exceeds the numeric range of the planner");
    }
    const problem: FlightProblem = {
        velocity: scale(velocity, 1 / velocity_unit),
        final_velocity: [0, 0],
        displacement: scale(displacement, 1 / length_unit),
        orientation: ship.orientation,
        turn_time: 1 / (ship.max_rotate_speed * time_unit),
    };
    // Solve to centimetres or better, keeping ample margin for replay rounding.
    const tolerance = Math.min(1e-10, 0.01 / length_unit);
    let best = safe_seed(problem);

    // Two burns often suffice. Start from several directions because the
    // shortest-turn function has corners at zero and at half a revolution.
    const guess = subtract(problem.displacement, scale(problem.velocity, 0.4));
    for (const seed of [guess, scale(guess, -1),
        [guess[1], -guess[0]] as Vector, [-guess[1], guess[0]] as Vector])
    {
        const candidate = meet_target(problem, [seed], tolerance);
        if (candidate && candidate.time < best.time) {
            best = candidate;
        }
    }

    best = shorten(problem, best, tolerance);
    while (best.via.length < 3) {
        const velocities: Vector[] = [problem.velocity, ...best.via, [0, 0]];
        let improved = best;
        // Splitting a burn preserves the starting trajectory. Optimize each
        // split, so extra turns are kept only when their time cost pays off.
        for (let i = 0; i < velocities.length - 1; i += 1) {
            const left = velocities[i]!;
            const right = velocities[i + 1]!;
            const middle: Vector = [(left[0] + right[0]) / 2, (left[1] + right[1]) / 2];
            const via = [...best.via.slice(0, i), middle, ...best.via.slice(i)];
            const candidate = shorten(problem, evaluate(problem, via), tolerance);
            if (candidate.time < improved.time - 1e-7) {
                improved = candidate;
            }
        }
        if (improved === best) {
            break;
        }
        best = improved;
    }

    const relaxed = relax_arrival(problem, best,
        (POSITION_TOLERANCE - 0.001) / length_unit,
        (VELOCITY_TOLERANCE - 1e-6) / velocity_unit,
        Math.min(tolerance, 0.0001 / length_unit));
    const result = schedule(ship, relaxed.trajectory.via.map(v => scale(v, velocity_unit)),
        target.velocity, scale(relaxed.problem.final_velocity, velocity_unit));
    const candidates = [result];
    if (simple) {
        candidates.push(simple);
    }
    candidates.push(schedule(ship, safe_seed(problem).via.map(v => scale(v, velocity_unit)),
        target.velocity, [0, 0]));
    const end = (plan: FlightPlan): number => {
        const last = plan.maneuvers[plan.maneuvers.length - 1];
        return last ? maneuver_end(last) : ship.position.timestamp;
    };
    candidates.sort((a, b) => end(a) - end(b) || a.maneuvers.length - b.maneuvers.length);
    // Check the actual timestamped replay as well as the normalized equations.
    // Large absolute coordinates/timestamps can lose floating-point precision.
    for (const candidate of candidates) {
        if (reaches_target(ship, target, candidate)) {
            return candidate;
        }
    }
    throw new RangeError("cannot represent an arrival within the required tolerances");
}
