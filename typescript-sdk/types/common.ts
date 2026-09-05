import { create } from "@bufbuild/protobuf";
import * as proto from "#sdk/CommonTypes_pb.js";

export type Point = [number, number];
export type Vector = [number, number];

export type Position = {
    timestamp: bigint;
    point: Point;
    velocity: Vector;
};

export type ServerTimestamp = {
    real_us: bigint;
    ingame_us: bigint;
};

export type Kinematics = {
    x: number;
    y: number;
    vx: number;
    vy: number;
};

export function asUint64(value: bigint | number): bigint {
    return typeof value === "bigint" ? value : BigInt(value);
}

export function positionFromKinematics(
    kinematics: Kinematics | undefined,
    timestamp: bigint): Position
{
    return {
        timestamp,
        point: [kinematics?.x ?? 0, kinematics?.y ?? 0],
        velocity: [kinematics?.vx ?? 0, kinematics?.vy ?? 0],
    };
}

export function positionFromProtobuf(
    position: proto.Position | undefined,
    timestamp: bigint): Position
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
