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
    const dt_sec = (timestamp - position.timestamp) / 1e6;
    const velocity = position.velocity;
    const acc = position.acc;
    const new_x = position.x + velocity.x * dt_sec + 0.5 * acc.x * dt_sec * dt_sec;
    const new_y = position.y + velocity.y * dt_sec + 0.5 * acc.y * dt_sec * dt_sec;
    return {
        timestamp: timestamp,
        x: new_x,
        y: new_y,
        velocity: { x: velocity.x, y: velocity.y },
        acc: { x: acc.x, y: acc.y },
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

export function pack_position(position: Position) {
    return [
        position.timestamp,
        position.x,
        position.y,
        position.velocity.x,
        position.velocity.y,
        position.acc.x,
        position.acc.y,
    ] as const;
}

export function unpack_position(packed: ReturnType<typeof pack_position>)
: Position
{
    return {
        timestamp: packed[0],
        x: packed[1],
        y: packed[2],
        velocity: { x: packed[3], y: packed[4] },
        acc: { x: packed[5], y: packed[6] },
    };
}