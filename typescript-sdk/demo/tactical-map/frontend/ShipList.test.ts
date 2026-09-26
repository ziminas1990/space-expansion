import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { expect, test } from "vitest";
import { PlayerShip } from "../common/domain/player_ship.js";
import type { Position } from "../common/domain/position.js";
import { World } from "../common/domain/world.js";
import { noop_logger } from "../common/logger.js";
import { group_player_ships, ShipList } from "./ShipList.js";

const position: Position = {
    timestamp: 0,
    x: 0,
    y: 0,
    velocity: { x: 0, y: 0 },
    acc: { x: 0, y: 0 },
};

test("groups player ships by blueprint in type name order", () => {
    const ships = [
        new PlayerShip("scout-1", position, 10, "Tiny-Scout"),
        new PlayerShip("miner-1", position, 10, "Miner"),
        new PlayerShip("scout-2", position, 10, "Tiny-Scout"),
    ];
    const groups = group_player_ships(ships);
    expect(groups.map(([type]) => type)).toEqual(["Miner", "Tiny-Scout"]);
    expect(groups[1]?.[1].map((ship) => ship.get_id()))
        .toEqual(["scout-1", "scout-2"]);
});

test("keeps the selected ship listed when it is outside the visible area", () => {
    const world = new World(noop_logger);
    world.update({
        type: "add_player_ship",
        ship: new PlayerShip("selected-scout", position, 10, "Tiny-Scout"),
    });
    world.update({
        type: "add_player_ship",
        ship: new PlayerShip("other-miner", position, 10, "Miner"),
    });

    const html = renderToStaticMarkup(createElement(ShipList, {
        world,
        visible_ship_ids: new Set<string>(),
        selected_ship_id: "selected-scout",
        followed_ship_id: undefined,
        on_select: () => {},
    }));

    expect(html).toContain("selected-scout");
    expect(html).toContain("Tiny-Scout");
    expect(html).not.toContain("other-miner");
    expect(html).not.toContain("Miner");
});
