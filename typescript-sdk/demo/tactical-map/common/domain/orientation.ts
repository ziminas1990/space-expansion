import { Vector2D } from "./position.js";

// Sampled facing plus the turn rate inferred from the previous sample.
// `timestamp` is ingame time in microseconds, the same clock as Position.
// `turn` is radians per second. Positive rotation moves +X toward +Y.
export type Orientation = {
    timestamp: number;
    x: number;
    y: number;
    turn: number;
}

export function orientation_from(vector: Vector2D, timestamp: number): Orientation {
    return {
        timestamp,
        x: vector.x,
        y: vector.y,
        turn: 0,
    };
}

export function copy_orientation(orientation: Orientation): Orientation {
    return {
        timestamp: orientation.timestamp,
        x: orientation.x,
        y: orientation.y,
        turn: orientation.turn,
    };
}

// Shortest signed angle from `from` to `to`, in radians.
// Undefined when either vector has no direction.
function shortest_angle(from: Vector2D, to: Vector2D): number | undefined {
    if (!(Math.hypot(from.x, from.y) > 0) || !(Math.hypot(to.x, to.y) > 0)) {
        return undefined;
    }
    let delta = Math.atan2(to.y, to.x) - Math.atan2(from.y, from.x);
    if (delta > Math.PI) {
        delta -= 2 * Math.PI;
    } else if (delta < -Math.PI) {
        delta += 2 * Math.PI;
    }
    return delta;
}

function rotate(vector: Vector2D, angle: number): Vector2D {
    const cos = Math.cos(angle);
    const sin = Math.sin(angle);
    return {
        x: vector.x * cos - vector.y * sin,
        y: vector.x * sin + vector.y * cos,
    };
}

export function predict_orientation(orientation: Orientation, timestamp: number)
: Orientation
{
    const dt_sec = (timestamp - orientation.timestamp) / 1e6;
    const rotated = rotate(orientation, orientation.turn * dt_sec);
    return {
        timestamp,
        x: rotated.x,
        y: rotated.y,
        turn: orientation.turn,
    };
}

// Replace `turn` from the facing change over ingame time.
// An older sample is ignored. A sample that is not later keeps `turn` at 0.
export function update_orientation(
    previous: Orientation | undefined,
    next: Orientation,
): Orientation {
    if (previous !== undefined && next.timestamp < previous.timestamp) {
        return copy_orientation(previous);
    }
    const orientation = copy_orientation(next);
    orientation.turn = 0;
    if (previous === undefined) {
        return orientation;
    }
    const dt_sec = (next.timestamp - previous.timestamp) / 1e6;
    if (!(dt_sec > 0)) {
        return orientation;
    }
    const angle = shortest_angle(previous, next);
    if (angle === undefined) {
        return orientation;
    }
    orientation.turn = angle / dt_sec;
    return orientation;
}

export function apply_orientation(
    current: Orientation | undefined,
    vector: Vector2D,
    timestamp: number,
): Orientation {
    return update_orientation(current, orientation_from(vector, timestamp));
}

export type OrientationPacked = readonly [number, number, number, number];

export function pack_orientation(
    orientation: Orientation | undefined,
): OrientationPacked | null {
    if (orientation === undefined) {
        return null;
    }
    return [orientation.timestamp, orientation.x, orientation.y, orientation.turn];
}

export function unpack_orientation(
    packed: OrientationPacked | null | undefined,
): Orientation | undefined {
    if (packed == null) {
        return undefined;
    }
    return {
        timestamp: packed[0],
        x: packed[1],
        y: packed[2],
        turn: packed[3],
    };
}
