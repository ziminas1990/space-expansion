import type { Vector2D } from "../../common/domain/position.js";

// A picture is drawn nose-up. The map uses the same axes as the world:
// +X to the right, +Y down the screen. Nose-up is therefore world (0, -1).
// Pixi rotation is clockwise, so atan2(x, -y) turns (0, -1) onto (x, y).
// A missing or zero orientation stays nose-up.
export function picture_rotation(orientation: Vector2D | undefined): number {
    if (orientation === undefined) {
        return 0;
    }
    if (!(Math.hypot(orientation.x, orientation.y) > 0)) {
        return 0;
    }
    return Math.atan2(orientation.x, -orientation.y);
}
