import * as midlevel from "@spx/sdk/midlevel";
import { Status } from "@spx/sdk/types";
import { IWorld } from "./interfaces.js";
import { Logger } from "../log.js";
import { Ship } from "./ship.js";
import { SystemClock } from "./system_clock.js";


export class RootCommutator {

    // slot_id -> module_info
    private modules: Map<number, midlevel.ModuleInfo> = new Map();
    // slot_id -> ship controller
    private ships: Map<number, Ship> = new Map();
    private system_clock?: { slot_id: number, controller: SystemClock };

    private initialized: boolean = false;
    private stopped: boolean = false;
    private monitoring_task: Promise<void>;

    constructor(
        private readonly commutator: midlevel.Commutator,
        private readonly world: IWorld,
        private readonly logger: Logger,
    ) {
        this.monitoring_task = Promise.resolve();
    }

    async initialize(): Promise<Status> {
        const [status, modules] = await this.commutator.get_all_modules_info();
        if (!status.is_ok()) {
            return status.wrap("Failed to get all modules info");
        }
        for (const module of modules!) {
            const status = await this.on_module_attached(module);
            if (!status.is_ok()) {
                this.logger.error(`Failed to handle module attached: ${status.what()}`);
            }
        }
        this.monitoring_task = this.monitoring();
        this.initialized = true;
        return Status.ok();
    }

    verify(): Status {
        if (!this.initialized) {
            return Status.fail("Root commutator is not initialized");
        }
        if (!this.system_clock) {
            return Status.fail("SystemClock is not running");
        }
        return Status.ok();
    }

    async stop(reason: string): Promise<Status> {
        this.logger.info(`Stopping root commutator: ${reason}`);
        this.stopped = true;
        await this.monitoring_task;
        await Promise.all([
            ...this.ships.values().map(ship => ship.stop()),
            this.system_clock?.controller.stop(),
        ]);
        this.ships.clear();
        this.system_clock = undefined;
        await this.commutator.terminate();
        return Status.ok();
    }

    is_stopped(): boolean {
        return this.stopped;
    }

    private async monitoring() {
        const status = await this.commutator.monitoring(this.handle_update.bind(this));
        if (!status.is_ok()) {
            this.logger.error(`Monitoring error: ${status.what()}`);
        }
        this.logger.info("Monitoring finished");
        this.stopped = true;
        this.monitoring_task = Promise.resolve();
    }

    private async handle_update(update: midlevel.CommutatorUpdate | undefined)
    : Promise<boolean> {
        if (update === undefined) {
            return !this.stopped;
        }
        if (update.module_attached) {
            await this.on_module_attached(update.module_attached);
        }
        if (update.module_detached !== undefined) {
            await this.on_module_detached(update.module_detached);
        }
        return !this.stopped;
    }

    private async on_module_attached(module_info: midlevel.ModuleInfo)
    : Promise<Status>
    {
        this.modules.set(module_info.slot_id, module_info);
        if (module_info.module_type === midlevel.ModuleType.SHIP) {
            return this.on_new_ship(module_info);
        }
        if (module_info.module_type === midlevel.ModuleType.SYSTEM_CLOCK) {
            return this.on_new_system_clock(module_info);
        }
        return Status.ok();
    }

    private async on_module_detached(slot_id: number): Promise<Status> {
        const module_info = this.modules.get(slot_id);
        if (!module_info) {
            return Status.ok();
        }
        this.modules.delete(slot_id);
        if (module_info.module_type === midlevel.ModuleType.SHIP) {
            return this.on_ship_detached(module_info);
        }
        if (module_info.module_type === midlevel.ModuleType.SYSTEM_CLOCK) {
            return this.on_system_clock_detached(module_info);
        }
        return Status.ok();
    }

    private async on_new_ship(info: midlevel.ModuleInfo): Promise<Status> {
        if (this.ships.has(info.slot_id)) {
            return Status.ok();
        }

        const ship = new Ship(
            new midlevel.Ship(info.open_session_cb),
            this.world,
            this.logger.child(info.module_name),
            info.module_name,
        );
        const status = await ship.initialize();
        if (!status.is_ok()) {
            return status;
        }
        this.ships.set(info.slot_id, ship);
        return Status.ok();
    }

    private async on_ship_detached(info: midlevel.ModuleInfo): Promise<Status> {
        const ship = this.ships.get(info.slot_id);
        if (ship) {
            await ship.stop();
            this.ships.delete(info.slot_id);
        }
        return Status.ok();
    }

    private async on_new_system_clock(info: midlevel.ModuleInfo): Promise<Status> {
        if (this.system_clock?.slot_id === info.slot_id) {
            return Status.ok();
        }

        const clock = new SystemClock(
            new midlevel.SystemClock(info.open_session_cb),
            this.world,
            this.logger.child(info.module_name),
            info.module_name,
        );
        const status = await clock.initialize();
        if (!status.is_ok()) {
            return status;
        }

        if (this.system_clock) {
            this.logger.warning(
                `Second SystemClock '${info.module_name}' at slot ${info.slot_id}; `
                + `replacing the one at slot ${this.system_clock.slot_id}`,
            );
            await this.system_clock.controller.stop();
        }
        this.system_clock = { slot_id: info.slot_id, controller: clock };
        return Status.ok();
    }

    private async on_system_clock_detached(info: midlevel.ModuleInfo): Promise<Status> {
        if (this.system_clock?.slot_id === info.slot_id) {
            await this.system_clock.controller.stop();
            this.system_clock = undefined;
        }
        return Status.ok();
    }
}