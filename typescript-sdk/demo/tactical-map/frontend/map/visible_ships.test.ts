import { expect, test } from "vitest";
import { PlayerShip } from "../../common/domain/player_ship.js";
import type { Position } from "../../common/domain/position.js";
import { World } from "../../common/domain/world.js";
import { noop_logger } from "../../common/logger.js";
import { create_camera, pan_by, zoom_at } from "./camera.js";
import { visible_player_ship_ids } from "./visible_ships.js";

const VIEWPORT = { width: 800, height: 600 };

function position(x: number, y: number): Position {
    return {
        timestamp: 0,
        x,
        y,
        velocity: { x: 0, y: 0 },
        acc: { x: 0, y: 0 },
    };
}

test("finds ships inside the current camera bounds after pan and zoom", () => {
    const world = new World(noop_logger);
    world.update({
        type: "add_player_ship",
        ship: new PlayerShip("inside", position(400, 0), 10, "Miner"),
    });
    world.update({
        type: "add_player_ship",
        ship: new PlayerShip("outside", position(500, 0), 10, "Miner"),
    });
    const camera = create_camera();
    expect([...visible_player_ship_ids(world, camera, VIEWPORT)]).toEqual(["inside"]);

    const panned = pan_by(camera, { x: -200, y: 0 });
    expect([...visible_player_ship_ids(world, panned, VIEWPORT)])
        .toEqual(["inside", "outside"]);

    const zoomed = zoom_at(panned, VIEWPORT, { x: 400, y: 300 }, 2);
    expect([...visible_player_ship_ids(world, zoomed, VIEWPORT)]).toEqual(["inside"]);
});
