import * as midlevel from "@spx/sdk/midlevel";
import { Status } from "@spx/sdk/types";
import { expect, test } from "vitest";
import { PlayerShip } from "../../common/domain/player_ship.js";
import { Shipyard } from "../../common/domain/shipyard.js";
import { World } from "../../common/domain/world.js";
import { noop_logger } from "../../common/logger.js";
import { ShipyardController } from "./shipyard.js";

test("maps monitoring events to the correct Shipyard and clears terminal builds", async () => {
    const world = new World(noop_logger);
    world.update({
        type: "add_player_ship",
        ship: new PlayerShip("Builder", {
            timestamp: 0, x: 0, y: 0,
            velocity: { x: 0, y: 0 }, acc: { x: 0, y: 0 },
        }, 10, "Station"),
    });
    world.update({
        type: "player_ship_modules_update", ship_id: "Builder",
        modules: [
            { slot_id: 1, type: "Shipyard", name: "Main bay" },
            { slot_id: 2, type: "Shipyard", name: "Other bay" },
        ],
    });
    let callback: midlevel.ShipyardMonitoringCallback | undefined;
    let finish: ((status: Status) => void) | undefined;
    const remote = {
        monitoring: async (on_event: midlevel.ShipyardMonitoringCallback) => {
            callback = on_event;
            return await new Promise<Status>((resolve) => { finish = resolve; });
        },
        terminate: async () => { finish?.(Status.ok()); },
    } as midlevel.Shipyard;
    const controller = new ShipyardController(
        remote, world, noop_logger, "Builder", 1, "Main bay",
    );
    const state = (slot_id: number) =>
        (world.get_player_ship("Builder")?.get_module(slot_id) as Shipyard).get_state();

    controller.start();
    await callback?.({ case: "idle" });
    expect(state(1)).toEqual({ status: "idle" });
    await callback?.({
        case: "build_started",
        build: { blueprint_name: "Ship/Miner", ship_name: "Ore One" },
    });
    await callback?.({
        case: "building_report",
        report: { status: "BUILD_FROZEN", progress: 0.34 },
    });
    expect(state(1)).toEqual({
        status: "frozen", blueprint_name: "Ship/Miner",
        ship_name: "Ore One", progress: 0.34,
    });
    expect(state(2)).toBeUndefined();
    await callback?.({
        case: "building_report",
        report: { status: "BUILD_IN_PROGRESS", progress: 0.8 },
    });
    expect(state(1)?.status).toBe("building");
    await callback?.({
        case: "building_complete",
        ship: { ship_name: "Ore One", slot_id: 5 },
    });
    expect(state(1)).toEqual({ status: "idle" });

    await callback?.({
        case: "build_started",
        build: { blueprint_name: "Ship/Scout", ship_name: "Eye" },
    });
    await callback?.({
        case: "building_report",
        report: { status: "BUILD_CANCELED", progress: 0.1 },
    });
    expect(state(1)).toEqual({ status: "idle" });

    await controller.stop();
    expect(await callback?.({ case: "idle" })).toBe(false);
});
