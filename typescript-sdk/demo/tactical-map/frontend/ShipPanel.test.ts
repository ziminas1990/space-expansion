import { createElement } from "react";
import { renderToStaticMarkup } from "react-dom/server";
import { expect, test } from "vitest";
import { PlayerShip } from "../common/domain/player_ship.js";
import { ResourceContainer } from "../common/domain/resource_container.js";
import type { Position } from "../common/domain/position.js";
import {
    group_modules,
    module_expansion_key,
    ShipPanel,
    toggle_module,
    toggle_module_type,
} from "./ShipPanel.js";

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
        expanded_modules: new Set(),
        on_toggle_type: () => {},
        on_toggle_module: () => {},
    }));
    expect(html).toContain("Scout");
    expect(html).toContain("Tiny-Scout");
    expect(html).toContain("RCS");
    expect(html).toContain('aria-expanded="false"');
    expect(html).not.toContain("Left RCS");
});

test("shares expanded module types within a blueprint group", () => {
    // 1. open one type for each of two blueprint groups
    let expanded = toggle_module_type(new Map(), "Tiny-Scout", "RCS");
    expanded = toggle_module_type(expanded, "Miner", "HoverEngine");

    // 2. render a different ship from the first group with its shared settings
    const scout = new PlayerShip("Other scout", position, 10, "Tiny-Scout");
    scout.set_modules([{ slot_id: 4, type: "RCS", name: "Left RCS" }]);
    const html = renderToStaticMarkup(createElement(ShipPanel, {
        ship: scout,
        expanded_types: expanded.get(scout.get_blueprint_name()) ?? new Set(),
        expanded_modules: new Set(),
        on_toggle_type: () => {},
        on_toggle_module: () => {},
    }));
    expect(html).toContain('aria-expanded="true"');
    expect(html).toContain("Left RCS");
    expect(expanded.get("Miner")?.has("HoverEngine")).toBe(true);

    // 3. close the first group without changing the other blueprint
    expanded = toggle_module_type(expanded, "Tiny-Scout", "RCS");
    expect(expanded.get("Tiny-Scout")).toBeUndefined();
    expect(expanded.get("Miner")?.has("HoverEngine")).toBe(true);
});

test("shows container fill and sorts its contents by descending amount", () => {
    const ship = new PlayerShip("Miner", position, 10, "Miner");
    ship.set_modules([{ slot_id: 7, type: "ResourceContainer", name: "Ore hold" }]);
    const cargo = ship.get_module(7);
    expect(cargo).toBeInstanceOf(ResourceContainer);
    if (!(cargo instanceof ResourceContainer)) {
        throw new Error("Expected resource container");
    }
    cargo.update_content({
        volume: 200,
        used: 50,
        resources: [
            { resource_type: "ice", amount: 3.4 },
            { resource_type: "metals", amount: 1200.6 },
            { resource_type: "silicates", amount: 400.5 },
        ],
    });

    const html = renderToStaticMarkup(createElement(ShipPanel, {
        ship,
        expanded_types: new Set(["ResourceContainer"]),
        expanded_modules: new Set([module_expansion_key(ship.get_modules()[0]!)]),
        on_toggle_type: () => {},
        on_toggle_module: () => {},
    }));
    expect(html).toContain('<details class="resource-container-widget" open=""');
    expect(html).toContain("<summary><span>Ore hold</span><span class=\"resource-container-fill\">25%</span></summary>");
    expect(html.indexOf("metals")).toBeLessThan(html.indexOf("silicates"));
    expect(html.indexOf("silicates")).toBeLessThan(html.indexOf("ice"));
    expect(html).toContain("1,201 kg");
    expect(html).toContain("401 kg");
    expect(html).toContain("3 kg");
    expect(html).not.toContain("1,200.6");
});

test("shares expanded modules by name within a blueprint group", () => {
    const main = { slot_id: 7, type: "ResourceContainer", name: "Main hold" };
    const reserve = { slot_id: 8, type: "ResourceContainer", name: "Reserve hold" };
    const main_key = module_expansion_key(main);
    const reserve_key = module_expansion_key(reserve);
    let expanded = toggle_module(new Map(), "Miner", main_key);
    expanded = toggle_module(expanded, "Miner", reserve_key);
    expanded = toggle_module(expanded, "Tiny-Scout", main_key);
    expect([...expanded.get("Miner") ?? []]).toEqual([main_key, reserve_key]);
    expect([...expanded.get("Tiny-Scout") ?? []]).toEqual([main_key]);

    expanded = toggle_module(expanded, "Miner", main_key);
    expect([...expanded.get("Miner") ?? []]).toEqual([reserve_key]);
    expect([...expanded.get("Tiny-Scout") ?? []]).toEqual([main_key]);
    const expanded_types = toggle_module_type(new Map(), "Miner", "ResourceContainer");

    const ship = new PlayerShip("Other miner", position, 10, "Miner");
    ship.set_modules([
        { ...main, slot_id: 17 },
        { ...reserve, slot_id: 18 },
    ]);
    const html = renderToStaticMarkup(createElement(ShipPanel, {
        ship,
        expanded_types: expanded_types.get(ship.get_blueprint_name()) ?? new Set(),
        expanded_modules: expanded.get(ship.get_blueprint_name()) ?? new Set(),
        on_toggle_type: () => {},
        on_toggle_module: () => {},
    }));
    expect(html).toContain('<details class="resource-container-widget"><summary><span>Main hold');
    expect(html).toContain('<details class="resource-container-widget" open=""><summary><span>Reserve hold');
});
