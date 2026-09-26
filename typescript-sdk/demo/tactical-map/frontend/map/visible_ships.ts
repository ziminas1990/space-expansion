import { predict_position } from "../../common/domain/position.js";
import type { World } from "../../common/domain/world.js";
import type { Camera, Viewport } from "./camera.js";

export function visible_player_ship_ids(
    world: World,
    camera: Camera,
    viewport: Viewport,
): ReadonlySet<string> {
    const ids = new Set<string>();
    if (viewport.width <= 0 || viewport.height <= 0) {
        return ids;
    }
    const half_width = viewport.width / (2 * camera.scale);
    const half_height = viewport.height / (2 * camera.scale);
    const now = world.now();
    for (const ship of world.get_player_ships()) {
        const position = ship.get_position();
        const predicted = predict_position(position, now ?? position.timestamp);
        if (Math.abs(predicted.x - camera.center.x) <= half_width
            && Math.abs(predicted.y - camera.center.y) <= half_height)
        {
            ids.add(ship.get_id());
        }
    }
    return ids;
}
