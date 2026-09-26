import * as midlevel from "@spx/sdk/midlevel";
import { Status } from "@spx/sdk/types";
import { afterEach, expect, test, vi } from "vitest";
import { noop_logger } from "../../common/logger.js";
import { World } from "../../common/domain/world.js";
import { ResourceContainer } from "../../common/domain/resource_container.js";
import { HoverEngine } from "../../common/domain/hover_engine.js";
import { RCS } from "../../common/domain/rcs.js";
import { Shipyard as ShipyardModule } from "../../common/domain/shipyard.js";
import { Ship } from "./ship.js";

afterEach(() => vi.restoreAllMocks());

test("follows cargo, propulsion, and Shipyard modules through attach, change, and detach", async () => {
    const cargo_callbacks: midlevel.ResourceContainerMonitoringCallback[] = [];
    const engine_callbacks: midlevel.HoverEngineMonitoringCallback[] = [];
    const rcs_callbacks: midlevel.RCSMonitoringCallback[] = [];
    const shipyard_callbacks: midlevel.ShipyardMonitoringCallback[] = [];
    const cargo_finishes: ((status: Status) => void)[] = [];
    const engine_finishes: ((status: Status) => void)[] = [];
    const rcs_finishes: ((status: Status) => void)[] = [];
    const shipyard_finishes: ((status: Status) => void)[] = [];

    vi.spyOn(midlevel.ResourceContainer.prototype, "monitoring")
        .mockImplementation(async (callback) => {
            cargo_callbacks.push(callback);
            return await new Promise<Status>((resolve) => { cargo_finishes.push(resolve); });
        });
    vi.spyOn(midlevel.ResourceContainer.prototype, "terminate")
        .mockImplementation(async () => { cargo_finishes.shift()?.(Status.ok()); });
    vi.spyOn(midlevel.HoverEngine.prototype, "monitoring")
        .mockImplementation(async (callback) => {
            engine_callbacks.push(callback);
            return await new Promise<Status>((resolve) => { engine_finishes.push(resolve); });
        });
    vi.spyOn(midlevel.HoverEngine.prototype, "terminate")
        .mockImplementation(async () => { engine_finishes.shift()?.(Status.ok()); });
    vi.spyOn(midlevel.RCS.prototype, "monitoring")
        .mockImplementation(async (callback) => {
            rcs_callbacks.push(callback);
            return await new Promise<Status>((resolve) => { rcs_finishes.push(resolve); });
        });
    vi.spyOn(midlevel.RCS.prototype, "terminate")
        .mockImplementation(async () => { rcs_finishes.shift()?.(Status.ok()); });
    vi.spyOn(midlevel.Shipyard.prototype, "monitoring")
        .mockImplementation(async (callback) => {
            shipyard_callbacks.push(callback);
            return await new Promise<Status>((resolve) => { shipyard_finishes.push(resolve); });
        });
    vi.spyOn(midlevel.Shipyard.prototype, "terminate")
        .mockImplementation(async () => { shipyard_finishes.shift()?.(Status.ok()); });

    const module_info = (slot_id: number, module_type: string, module_name: string)
        : midlevel.ModuleInfo => ({
            slot_id,
            module_type,
            module_name,
            blueprint_name: module_name,
            open_session_cb: async () => [Status.fail("unused"), undefined],
        });
    const modules = [
        module_info(1, midlevel.ModuleType.RESOURCE_CONTAINER, "Main hold"),
        module_info(2, midlevel.ModuleType.RESOURCE_CONTAINER, "Reserve hold"),
        module_info(3, midlevel.ModuleType.HOVER_ENGINE, "Main engine"),
        module_info(4, midlevel.ModuleType.RCS, "Port RCS"),
        module_info(6, midlevel.ModuleType.SHIPYARD, "Main bay"),
    ];
    let module_update:
        ((update: midlevel.CommutatorUpdate | undefined) => Promise<boolean>) | undefined;
    let finish_ship: (() => void) | undefined;
    let finish_commutator: (() => void) | undefined;
    const commutator = {
        get_all_modules_info: async () => [Status.ok(), modules],
        monitoring: async (callback: (update: midlevel.CommutatorUpdate | undefined) => Promise<boolean>) => {
            module_update = callback;
            return await new Promise<Status>((resolve) => {
                finish_commutator = () => resolve(Status.ok());
            });
        },
    };
    const remote = {
        get_state: async () => [Status.ok(), {
            position: {
                timestamp: 1,
                point: [0, 0],
                velocity: [0, 0],
            },
            orientation: [1, 0],
        }],
        get_specification: async () => [Status.ok(), { radius: 10 }],
        commutator: () => commutator,
        monitoring: async () => await new Promise<Status>((resolve) => {
            finish_ship = () => resolve(Status.ok());
        }),
        terminate: async () => {
            finish_ship?.();
            finish_commutator?.();
        },
    } as unknown as midlevel.Ship;
    const world = new World(noop_logger);
    const controller = new Ship(remote, world, noop_logger, "Miner", "Miner");
    expect((await controller.initialize()).is_ok()).toBe(true);
    expect(cargo_callbacks).toHaveLength(2);
    expect(engine_callbacks).toHaveLength(1);
    expect(rcs_callbacks).toHaveLength(1);
    expect(shipyard_callbacks).toHaveLength(1);

    await cargo_callbacks[0]?.({ timestamp: 1, volume: 100, used: 25,
        resources: [{ resource_type: "metals", amount: 500 }] });
    await cargo_callbacks[1]?.({ timestamp: 1, volume: 50, used: 0, resources: [] });
    await engine_callbacks[0]?.(40);
    await rcs_callbacks[0]?.({ x: -1, y: 0, thrust: 10 });
    await shipyard_callbacks[0]?.({ case: "idle" });
    await shipyard_callbacks[0]?.({
        case: "build_started",
        build: { blueprint_name: "Ship/Miner", ship_name: "Ore One" },
    });
    await shipyard_callbacks[0]?.({
        case: "building_report",
        report: { status: "BUILD_FROZEN", progress: 0.4 },
    });
    const ship = world.get_player_ship("Miner")!;
    const main = ship.get_module(1);
    const reserve = ship.get_module(2);
    const engine = ship.get_module(3);
    const rcs = ship.get_module(4);
    const shipyard = ship.get_module(6);
    expect(main).toBeInstanceOf(ResourceContainer);
    expect(reserve).toBeInstanceOf(ResourceContainer);
    expect(engine).toBeInstanceOf(HoverEngine);
    expect(rcs).toBeInstanceOf(RCS);
    expect(shipyard).toBeInstanceOf(ShipyardModule);
    expect((main as ResourceContainer).get_content()).toEqual({
        volume: 100, used: 25,
        resources: [{ resource_type: "metals", amount: 500 }],
    });
    expect((reserve as ResourceContainer).get_content())
        .toEqual({ volume: 50, used: 0, resources: [] });
    expect((engine as HoverEngine).get_thrust()).toBe(40);
    expect((rcs as RCS).get_thrust())
        .toEqual({ thrust: 10, direction: { x: -1, y: 0 } });
    expect((shipyard as ShipyardModule).get_state()).toEqual({
        status: "frozen", blueprint_name: "Ship/Miner",
        ship_name: "Ore One", progress: 0.4,
    });

    await cargo_callbacks[0]?.({ timestamp: 2, volume: 100, used: 40, resources: [] });
    await engine_callbacks[0]?.(0);
    await rcs_callbacks[0]?.({ x: 0, y: 1, thrust: 20 });
    expect((main as ResourceContainer).get_content())
        .toEqual({ volume: 100, used: 40, resources: [] });
    expect((engine as HoverEngine).get_thrust()).toBe(0);
    expect((rcs as RCS).get_thrust())
        .toEqual({ thrust: 20, direction: { x: 0, y: 1 } });

    await module_update?.({ module_detached: 1 });
    expect(world.get_player_ship("Miner")?.get_modules().map((module) => module.name))
        .toEqual(["Reserve hold", "Main engine", "Port RCS", "Main bay"]);
    expect(await cargo_callbacks[0]?.({ timestamp: 3, volume: 100, used: 90, resources: [] }))
        .toBe(false);

    await module_update?.({
        module_attached: module_info(5, midlevel.ModuleType.RESOURCE_CONTAINER, "New hold"),
    });
    await cargo_callbacks[2]?.({ timestamp: 4, volume: 200, used: 10, resources: [] });
    expect(ship.get_module(5)).toBeInstanceOf(ResourceContainer);
    expect((ship.get_module(5) as ResourceContainer).get_content())
        .toEqual({ volume: 200, used: 10, resources: [] });

    await module_update?.({ module_detached: 6 });
    expect(ship.get_module(6)).toBeUndefined();
    expect(await shipyard_callbacks[0]?.({ case: "idle" })).toBe(false);
    await module_update?.({
        module_attached: module_info(7, midlevel.ModuleType.SHIPYARD, "New bay"),
    });
    await shipyard_callbacks[1]?.({ case: "idle" });
    expect((ship.get_module(7) as ShipyardModule).get_state()).toEqual({ status: "idle" });

    await controller.stop();
    expect(world.get_player_ship("Miner")).toBeUndefined();
});
