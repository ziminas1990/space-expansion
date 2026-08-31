import type { Point, Position, Vector } from "../../types/common.js";

export type Rect = {
    left: number;
    right: number;
    bottom: number;
    top: number;
};

export function distance(
    left: Position | Point,
    right: Position | Point,
): number {
    const [lx, ly] = pointOf(left);
    const [rx, ry] = pointOf(right);
    return Math.hypot(lx - rx, ly - ry);
}

export function almostEqualVector(
    left: Vector,
    right: Vector,
    delta = 0.001,
): boolean {
    const dx = left[0] - right[0];
    const dy = left[1] - right[1];
    return dx * dx + dy * dy < delta * delta;
}

export function almostEqualPosition(
    left: Position,
    right: Position,
    delta = 0.001,
): boolean {
    return distance(left, right) < delta
        && almostEqualVector(left.velocity, right.velocity, delta);
}

function pointOf(value: Position | Point): Point {
    return Array.isArray(value) ? value : value.point;
}
