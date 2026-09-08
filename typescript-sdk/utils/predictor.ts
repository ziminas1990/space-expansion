import type { Position } from "#sdk/types/common.js";

// Linear kinematics: p(t) = p0 + v * dt. Velocity is treated as constant.
export function predict_position(position: Position, at_us: number): Position {
    const dt_sec = (at_us - position.timestamp) / 1e6;
    return {
        timestamp: at_us,
        point: [
            position.point[0] + position.velocity[0] * dt_sec,
            position.point[1] + position.velocity[1] * dt_sec,
        ],
        velocity: [position.velocity[0], position.velocity[1]],
    };
}