import * as sdk from "@spx/sdk";
import { Position, Vector2D } from "../../common/domain/position.js";

export function convert_position(position: sdk.Position): Position {
    return {
        timestamp: position.timestamp,
        x: position.point[0],
        y: position.point[1],
        velocity: {
            x: position.velocity[0],
            y: position.velocity[1],
        },
        acc: { x: 0, y: 0 },  // unknown from SDK
    }
}

export function convert_orientation(orientation: sdk.Vector): Vector2D {
    return {
        x: orientation[0],
        y: orientation[1],
    };
}