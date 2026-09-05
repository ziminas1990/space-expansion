import * as proto from "#sdk/CommonTypes_pb.js";
import { Position, positionFromKinematics } from "./common.js";

export type ObjectType = "unknown" | "asteroid" | "ship";

export type PhysicalObject = {
    object_type: ObjectType;
    object_id: number;
    position: Position;
    radius: number;
};

export function objectTypeFromProtobuf(value: proto.ObjectType): ObjectType {
    switch (value) {
        case proto.ObjectType.OBJECT_ASTEROID: return "asteroid";
        case proto.ObjectType.OBJECT_SHIP: return "ship";
        default: return "unknown";
    }
}

export function objectTypeToProtobuf(value: ObjectType): proto.ObjectType {
    switch (value) {
        case "asteroid": return proto.ObjectType.OBJECT_ASTEROID;
        case "ship": return proto.ObjectType.OBJECT_SHIP;
        case "unknown": return proto.ObjectType.OBJECT_UNKNOWN;
    }
}

export function physicalObjectFromProtobuf(
    object: proto.PhysicalObject,
    timestamp: bigint): PhysicalObject
{
    return {
        object_type: objectTypeFromProtobuf(object.objectType),
        object_id: object.id,
        radius: object.r,
        position: positionFromKinematics(object, timestamp),
    };
}
