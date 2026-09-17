import * as sdk from "@spx/sdk";
import { Position } from "../domain/position.js";

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