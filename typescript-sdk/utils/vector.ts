import type { Vector } from "../types/common.js";

const ALMOST_NULL_DELTA = 0.001;
const COLLINEAR_DELTA = 0.02;

export function vecAbs(v: Vector): number {
    return Math.hypot(v[0], v[1]);
}

export function vecAbsSqr(v: Vector): number {
    return v[0] * v[0] + v[1] * v[1];
}

export function vecAdd(left: Vector, right: Vector): Vector {
    return [left[0] + right[0], left[1] + right[1]];
}

export function vecSub(left: Vector, right: Vector): Vector {
    return [left[0] - right[0], left[1] - right[1]];
}

export function vecScale(v: Vector, k: number): Vector {
    return [v[0] * k, v[1] * k];
}

export function vecNeg(v: Vector): Vector {
    return [-v[0], -v[1]];
}

export function vecDot(left: Vector, right: Vector): number {
    return left[0] * right[0] + left[1] * right[1];
}

export function almostNull(v: Vector, delta = ALMOST_NULL_DELTA): boolean {
    return vecAbsSqr(v) < delta;
}

export function collinear(left: Vector, right: Vector): boolean {
    return Math.abs(left[0] * right[1] - left[1] * right[0]) < COLLINEAR_DELTA;
}

export function codirected(left: Vector, right: Vector): boolean {
    return collinear(left, right)
        && left[0] * right[0] >= 0
        && left[1] * right[1] >= 0;
}

export function setLength(v: Vector, length: number): Vector {
    const mag = vecAbs(v);
    if (mag === 0) {
        return [0, 0];
    }
    return vecScale(v, length / mag);
}

export function cosa(left: Vector, right: Vector): number {
    const denom = vecAbs(left) * vecAbs(right);
    if (denom === 0) {
        return 0;
    }
    return vecDot(left, right) / denom;
}

export function decompose(
    v: Vector,
    other: Vector,
): [Vector, Vector] {
    if (vecAbs(v) < 0.001) {
        return [[0, 0], [0, 0]];
    }
    const longitudinal = setLength(other, vecAbs(v) * cosa(v, other));
    return [longitudinal, vecSub(v, longitudinal)];
}
