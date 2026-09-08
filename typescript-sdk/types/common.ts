import { create } from "@bufbuild/protobuf";
import * as proto from "#sdk/CommonTypes_pb.js";

export type Point = [number, number];
export type Vector = [number, number];

export type Position = {
    timestamp: number;
    point: Point;
    velocity: Vector;
};

export type ServerTimestamp = {
    real_us: number;
    ingame_us: number;
};

export type Kinematics = {
    x: number;
    y: number;
    vx: number;
    vy: number;
};

// Convert a protobuf uint64 (bigint) into a JS number.
export function asNumber(value: bigint | number): number {
    return Number(value);
}

export function positionFromKinematics(
    kinematics: Kinematics | undefined,
    timestamp: number): Position
{
    return {
        timestamp,
        point: [kinematics?.x ?? 0, kinematics?.y ?? 0],
        velocity: [kinematics?.vx ?? 0, kinematics?.vy ?? 0],
    };
}

export function positionFromProtobuf(
    position: proto.Position | undefined,
    timestamp: number): Position
{
    return positionFromKinematics(position, timestamp);
}

export function positionToProtobuf(position: Position): proto.Position {
    return create(proto.PositionSchema, {
        x: position.point[0],
        y: position.point[1],
        vx: position.velocity[0],
        vy: position.velocity[1],
    });
}
