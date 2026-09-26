import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { expect, test } from "vitest";
import { PlayerShip } from "../common/domain/player_ship.js";
import type { Position } from "../common/domain/position.js";
import { group_modules, ShipPanel, toggle_module_type } from "./ShipPanel.js";

const position: Position = {
    timestamp: 0,
    x: 0,
    y: 0,
    velocity: { x: 0, y: 0 },
    acc: { x: 0, y: 0 },
};

test("groups every installed module by type", () => {
    // 1. give a ship multiple modules of the same type and one of another type
    const ship = new PlayerShip("Scout", position, 10, "Tiny-Scout");
    ship.set_modules([
        { slot_id: 1, type: "RCS", name: "Left RCS" },
        { slot_id: 2, type: "HoverEngine", name: "Main engine" },
        { slot_id: 3, type: "RCS", name: "Right RCS" },
    ]);

    // 2. confirm every module appears once in its type group
    const groups = group_modules(ship.get_modules());
    expect(groups.map(([type]) => type)).toEqual(["HoverEngine", "RCS"]);
    expect(groups[1]?.[1].map((module) => module.name))
        .toEqual(["Left RCS", "Right RCS"]);
});

test("renders a focused ship with all module groups initially collapsed", () => {
    // 1. create a ship with an installed module
    const ship = new PlayerShip("Scout", position, 10, "Tiny-Scout");
    ship.set_modules([{ slot_id: 1, type: "RCS", name: "Left RCS" }]);

    // 2. render the panel and check its visible headings and collapsed control
    const html = renderToStaticMarkup(createElement(ShipPanel, {
        ship,
        expanded_types: new Set(),
        on_toggle_type: () => {},
    }));
    expect(html).toContain("Scout");
    expect(html).toContain("Tiny-Scout");
    expect(html).toContain("RCS");
    expect(html).toContain('aria-expanded="false"');
    expect(html).not.toContain("Left RCS");
});

test("remembers expanded module types separately for each ship", () => {
    // 1. open one type on each of two ships
    let expanded = toggle_module_type(new Map(), "Scout", "RCS");
    expanded = toggle_module_type(expanded, "Miner", "HoverEngine");

    // 2. return to the first ship and render its remembered open group
    const scout = new PlayerShip("Scout", position, 10, "Tiny-Scout");
    scout.set_modules([{ slot_id: 1, type: "RCS", name: "Left RCS" }]);
    const html = renderToStaticMarkup(createElement(ShipPanel, {
        ship: scout,
        expanded_types: expanded.get("Scout") ?? new Set(),
        on_toggle_type: () => {},
    }));
    expect(html).toContain('aria-expanded="true"');
    expect(html).toContain("Left RCS");
    expect(expanded.get("Miner")?.has("HoverEngine")).toBe(true);

    // 3. close the first ship's group without changing the second ship's state
    expanded = toggle_module_type(expanded, "Scout", "RCS");
    expect(expanded.get("Scout")).toBeUndefined();
    expect(expanded.get("Miner")?.has("HoverEngine")).toBe(true);
});
