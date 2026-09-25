import type { Vector2D } from "../../common/domain/position.js";

// A picture is drawn nose-up. That pose is orientation (0, 1).
// Pixi rotation is clockwise, so atan2(x, y) turns the nose from up toward +X.
// A missing or zero orientation stays nose-up.
export function picture_rotation(orientation: Vector2D | undefined): number {
    if (orientation === undefined) {
        return 0;
    }
    if (!(Math.hypot(orientation.x, orientation.y) > 0)) {
        return 0;
    }
    return Math.atan2(orientation.x, orientation.y);
}
