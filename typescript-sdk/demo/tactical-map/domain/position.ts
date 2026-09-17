export type Vector2D = {
    x: number;
    y: number;
}

export type Position = {
    // Ingame timestamp in microseconds
    timestamp: number;
    x: number;
    y: number;
    velocity: Vector2D;
    acc: Vector2D
}

export type PositionTolerance = {
    distance: number;
    velocity?: number;
    acc?: number;
}

export function predict_position(position: Position, timestamp: number)
: Position
{
    const time_delta = timestamp - position.timestamp;
    const velocity = position.velocity;
    const acc = position.acc;
    const new_x = position.x + velocity.x * time_delta + 0.5 * acc.x * time_delta * time_delta;
    const new_y = position.y + velocity.y * time_delta + 0.5 * acc.y * time_delta * time_delta;
    return {
        timestamp: timestamp,
        x: new_x,
        y: new_y,
        velocity: velocity,
        acc: acc,
    }
}

export function equal_positions(
    a: Position, b: Position, tolerance: PositionTolerance
) : boolean
{
    const distance = (a.x - b.x) ** 2 + (a.y - b.y) ** 2;
    const velocity = (a.velocity.x - b.velocity.x) ** 2 + (a.velocity.y - b.velocity.y) ** 2;
    const acc = (a.acc.x - b.acc.x) ** 2 + (a.acc.y - b.acc.y) ** 2;

    if (distance > tolerance.distance ** 2) {
        return false;
    }

    if (tolerance.velocity !== undefined) {
        if (velocity > tolerance.velocity ** 2) {
            return false;
        }
    }

    if (tolerance.acc !== undefined) {
        if (acc > tolerance.acc ** 2) {
            return false;
        }
    }
    return true;
}

export function copy_position(position: Position): Position {
    return {
        timestamp: position.timestamp,
        x: position.x,
        y: position.y,
        velocity: { x: position.velocity.x, y: position.velocity.y },
        acc: { x: position.acc.x, y: position.acc.y },
    };
}
